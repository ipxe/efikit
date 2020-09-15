/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot device command-line tool
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <glib.h>
#include <efibootdev.h>
#include "efibootcli.h"

/** An EFI boot device subcommand */
struct efi_boot_command {
	/** Description */
	const char *description;
	/** Options */
	GOptionEntry *options;
	/** Execution method */
	int ( * exec ) ( int argc, char **argv );
};

/** Load option type */
static enum efi_boot_option_type type_value = EFIBOOT_TYPE_BOOT;

/** List of boot entries, with space for one additional entry */
static struct efi_boot_entry **entries;

/** Number of boot entries */
static unsigned int entry_count;

/** Boot order position flag */
static gboolean position_flag = FALSE;

/** Boot order position value */
static const char *position_value = NULL;

/** Variable name flag */
static gboolean name_flag = FALSE;

/** Attributes flag */
static gboolean attributes_flag = FALSE;

/** Attributes value */
static int attributes_value = 0;

/** Description flag */
static gboolean description_flag = FALSE;

/** Description value */
static const char *description_value = NULL;

/** First path flag */
static gboolean path_flag = FALSE;

/** All paths flag */
static gboolean paths_flag = FALSE;

/** Paths value */
static const char **paths_value = NULL;

/** Additional data flag */
static gboolean data_flag = FALSE;

/** Additional data value (base64-encoded) */
static const char *data_value = NULL;

/** Quiet flag */
static gboolean quiet_flag = FALSE;

/**
 * Parse load option type
 *
 * @v name		Option name
 * @v value		Option value
 * @v data		Opaque data
 * @v error		Error to fill in
 * @ret ok		Success indicator
 */
static gboolean parse_type ( const gchar *name, const gchar *value,
			     gpointer data, GError **error ) {

	( void ) name;
	( void ) data;

	/* Find type by name */
	type_value = efiboot_named_type ( value );
	if ( ! type_value ) {
		g_set_error ( error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
			      "Unknown type \"%s\"", value );
		return FALSE;
	}

	return TRUE;
}

/**
 * Parse boot order position argument
 *
 * @v text		Boot order position argument
 * @ret pos		Boot order position, or negative on error
 */
static int parse_position ( const char *arg ) {
	char *endp;
	int pos;

	/* Parse position */
	pos = strtol ( arg, &endp, 0 );
	if ( *endp || ( ! *arg ) ) {
		fprintf ( stderr, "Invalid position \"%s\"\n", arg );
		errno = EINVAL;
		return -1;
	}

	/* Treat negative positions as relative to list end */
	if ( pos < 0 )
		pos += entry_count;

	/* Check range */
	if ( ( pos < 0 ) || ( pos >= ( ( int ) entry_count ) ) ) {
		fprintf ( stderr, "Position %s out of range\n", arg );
		errno = ERANGE;
		return -1;
	}

	return pos;
}

/**
 * Parse boot identifier argument
 *
 * @v text		Boot identifier argument
 * @ret pos		Boot order position, or negative on error
 */
static int parse_id ( const char *arg ) {
	int pos;

	/* Try matching against variable names */
	for ( pos = 0 ; pos < ( ( int ) entry_count ) ; pos++ ) {
		if ( strcasecmp ( arg, efiboot_name ( entries[pos] ) ) == 0 )
			return pos;
	}

	/* Try parsing as boot order position */
	return parse_position ( arg );
}

/**
 * Show boot entry properties
 *
 * @v pos		Boot order position
 */
static void show_entry ( int pos ) {
	struct efi_boot_entry *entry = entries[pos];
	const char *sep = "";
	char *encoded;
	bool all;
	unsigned int count;
	unsigned int i;

	/* Show all fields if no fields are specified */
	all = ( ! ( position_flag || name_flag || attributes_flag ||
		    description_flag || path_flag || paths_flag ||
		    data_flag ) );

	/* Show boot order position, if applicable */
	if ( all || position_flag ) {
		printf ( "%s%d", sep, pos );
		sep = " ";
	}

	/* Show variable name, if applicable */
	if ( all || name_flag ) {
		printf ( "%s%s", sep, efiboot_name ( entry ) );
		sep = " ";
	}

	/* Show attributes, if applicable */
	if ( all || attributes_flag ) {
		printf ( "%s%08x", sep, efiboot_attributes ( entry ) );
		sep = " ";
	}

	/* Show description, if applicable */
	if ( all || description_flag ) {
		printf ( "%s%s", sep, efiboot_description ( entry ) );
		sep = " ";
	}

	/* Show path(s), if applicable */
	count = ( ( all || paths_flag ) ? efiboot_path_count ( entry ) :
		  path_flag ? 1 : 0 );
	for ( i = 0 ; i < count ; i++ ) {
		printf ( "%s%s", sep, efiboot_path_text ( entry, i ) );
		sep = " ";
	}

	/* Show additional data, if applicable */
	if ( ( all || data_flag ) && efiboot_data_len ( entry ) ) {
		encoded = g_base64_encode ( efiboot_data ( entry ),
					    efiboot_data_len ( entry ) );
		printf ( "%s%s", sep, encoded );
		sep = " ";
		g_free ( encoded );
	}

	/* Terminate line */
	printf ( "\n" );
}

/**
 * Set boot entry properties
 *
 * @v pos		Boot order position
 * @ret ok		Success indicator
 */
static int set_entry ( int pos ) {
	struct efi_boot_entry *entry = entries[pos];
	uint8_t *data = NULL;
	unsigned int path_count;
	size_t len;
	int new_pos;

	/* Set attributes */
	if ( attributes_value ) {
		if ( ! efiboot_set_attributes ( entry, attributes_value ) ) {
			perror ( "Could not set attributes" );
			goto err_set_attributes;
		}
	}

	/* Set description */
	if ( description_value ) {
		if ( ! efiboot_set_description ( entry, description_value ) ) {
			perror ( "Could not set description" );
			goto err_set_description;
		}
	}

	/* Set paths */
	if ( paths_value ) {
		path_count = 0;
		while ( paths_value[path_count] )
			path_count++;
		if ( ! efiboot_set_paths_text ( entry, paths_value,
						path_count ) ) {
			perror ( "Could not set paths" );
			goto err_set_paths_text;
		}
	}

	/* Set additional data, if applicable */
	if ( data_value ) {
		data = g_base64_decode ( data_value, &len );
		if ( ! data ) {
			fprintf ( stderr, "Invalid base64 additional data\n" );
			errno = EINVAL;
			goto err_base64_decode;
		}
		if ( ! efiboot_set_data ( entry, data, len ) ) {
			perror ( "Could not set additional data" );
			goto err_set_data;
		}
	}

	/* Set boot order position, if applicable */
	if ( position_value ) {
		new_pos = parse_position ( position_value );
		if ( new_pos < 0 )
			goto err_position;
		if ( new_pos > pos ) {
			memmove ( &entries[pos], &entries[pos + 1],
				  ( ( new_pos - pos ) *
				    sizeof ( entries[0] ) ) );
		} else if ( new_pos < pos ) {
			memmove ( &entries[new_pos + 1], &entries[new_pos],
				  ( ( pos - new_pos ) *
				    sizeof ( entries[0] ) ) );
		}
		entries[new_pos] = entry;
	}

	/* Save entries */
	if ( ! efiboot_save_all ( type_value, entries ) ) {
		perror ( "Could not save entries" );
		goto err_save_all;
	}

	/* Free decoded additional data */
	g_free ( data );

	return 1;

 err_save_all:
 err_position:
 err_set_data:
	g_free ( data );
 err_base64_decode:
 err_set_paths_text:
 err_set_description:
 err_set_attributes:
	return 0;
}

/**
 * Delete boot entry
 *
 * @v pos		Boot order position
 * @ret ok		Success indicator
 */
static int delete_entry ( int pos ) {
	struct efi_boot_entry *entry = entries[pos];

	/* Remove from boot order list */
	memmove ( &entries[pos], &entries[pos + 1],
		  ( ( entry_count - pos ) * sizeof ( entries[0] ) ) );
	entry_count--;

	/* Save boot order list */
	if ( ! efiboot_save_all ( type_value, entries ) ) {
		perror ( "Could not update boot order" );
		return 0;
	}

	/* Delete removed entry */
	if ( ! efiboot_del ( entry ) ) {
		perror ( "Could not delete entry" );
		return 0;
	}

	return 1;
}

/**
 * Show entries
 *
 * @v argc		Number of remaining command-line arguments
 * @v argv		Remaining command-line arguments
 * @ret ok		Success indicator
 */
static int efibootshow_exec ( int argc, char **argv ) {
	int pos;

	/* Show all or specified entries, as applicable */
	if ( argc == 1 ) {

		/* Show all entries */
		for ( pos = 0 ; pos < ( ( int ) entry_count ) ; pos++ )
			show_entry ( pos );

	} else {

		/* Show specified entries */
		for ( argv++, argc-- ; argc ; argv++, argc-- ) {

			/* Parse entry ID */
			pos = parse_id ( *argv );
			if ( pos < 0 )
				return 0;

			/* Show entry */
			show_entry ( pos );
		}
	}

	return 1;
}

/** "efibootshow" subcommand options */
static GOptionEntry efibootshow_options[] = {
	{ "type", 't', 0, G_OPTION_ARG_CALLBACK, parse_type,
	  "Load option type", "boot|driver|sysprep" },
	{ "position", 'o', 0, G_OPTION_ARG_NONE, &position_flag,
	  "Show boot order position", NULL },
	{ "name", 'n', 0, G_OPTION_ARG_NONE, &name_flag,
	  "Show variable name", NULL },
	{ "attributes", 'a', 0, G_OPTION_ARG_NONE, &attributes_flag,
	  "Show attributes", NULL },
	{ "description", 'd', 0, G_OPTION_ARG_NONE, &description_flag,
	  "Show description", NULL },
	{ "path", 'p', 0, G_OPTION_ARG_NONE, &path_flag,
	  "Show primary path", NULL },
	{ "paths", 'P', 0, G_OPTION_ARG_NONE, &paths_flag,
	  "Show all paths", NULL },
	{ "data", 'x', 0, G_OPTION_ARG_NONE, &data_flag,
	  "Show additional data", NULL },
	{}
};

/** "efibootshow" subcommand */
struct efi_boot_command efibootshow = {
	.description = "[<position>|<name>...] - Show EFI boot entries",
	.options = efibootshow_options,
	.exec = efibootshow_exec,
};

/**
 * Modify entry
 *
 * @v argc		Number of remaining command-line arguments
 * @v argv		Remaining command-line arguments
 * @ret ok		Success indicator
 */
static int efibootmod_exec ( int argc, char **argv ) {
	int pos;

	/* Identify entry */
	if ( argc < 2 ) {
		fprintf ( stderr, "Missing argument\n" );
		return 0;
	}
	if ( argc > 2 ) {
		fprintf ( stderr, "Too many arguments\n" );
		return 0;
	}
	pos = parse_id ( argv[1] );
	if ( pos < 0 )
		return 0;

	/* Update entry */
	if ( ! set_entry ( pos ) )
		return 0;

	return 1;
}

/** "efibootmod" subcommand options */
static GOptionEntry efibootmod_options[] = {
	{ "type", 't', 0, G_OPTION_ARG_CALLBACK, parse_type,
	  "Load option type", "boot|driver|sysprep" },
	{ "position", 'o', 0, G_OPTION_ARG_STRING, &position_value,
	  "Modify boot order position", "<position>" },
	{ "attributes", 'a', 0, G_OPTION_ARG_INT, &attributes_value,
	  "Modify attributes", "<attributes>" },
	{ "description", 'd', 0, G_OPTION_ARG_STRING, &description_value,
	  "Modify description", "<description>" },
	{ "path", 'p', 0, G_OPTION_ARG_STRING_ARRAY, &paths_value,
	  "Modify path(s)", "<path>..." },
	{ "data", 'x', 0, G_OPTION_ARG_STRING, &data_value,
	  "Modify additional data", "<base64 data>" },
	{}
};

/** "efibootmod" subcommand */
struct efi_boot_command efibootmod = {
	.description = "<position>|<name> - Modify EFI boot entry",
	.options = efibootmod_options,
	.exec = efibootmod_exec,
};

/**
 * Add entry
 *
 * @v argc		Number of remaining command-line arguments
 * @v argv		Remaining command-line arguments
 * @ret ok		Success indicator
 */
static int efibootadd_exec ( int argc, char **argv ) {
	struct efi_boot_entry *entry;

	/* Check arguments */
	if ( argc > 1 ) {
		fprintf ( stderr, "Too many arguments\n" );
		goto err_args;
	}
	( void ) argv;
	if ( ! description_value ) {
		fprintf ( stderr, "Must provide a description\n" );
		goto err_args;
	}
	if ( ! paths_value ) {
		fprintf ( stderr, "Must provide at least one path\n" );
		goto err_args;
	}

	/* Create new entry */
	entry = efiboot_new();
	if ( ! entry )
		goto err_new;

	/* Add to start of list */
	memmove ( &entries[1], &entries[0],
		  ( entry_count * sizeof ( entries[0] ) ) );
	entries[0] = entry;
	entry_count++;

	/* Set type */
	if ( ! efiboot_set_type ( entry, type_value ) ) {
		perror ( "Could not set type" );
		goto err_set_type;
	}

	/* Update entry */
	if ( ! set_entry ( 0 ) )
		goto err_set_entry;

	/* Show created variable name, if applicable */
	if ( ! quiet_flag )
		printf ( "%s\n", efiboot_name ( entry ) );

	/* Free new entry */
	efiboot_free ( entry );

	return 1;

 err_set_entry:
 err_set_type:
	efiboot_free ( entry );
 err_new:
 err_args:
	return 0;
}

/** "efibootadd" subcommand options */
static GOptionEntry efibootadd_options[] = {
	{ "type", 't', 0, G_OPTION_ARG_CALLBACK, parse_type,
	  "Load option type", "boot|driver|sysprep" },
	{ "position", 'o', 0, G_OPTION_ARG_STRING, &position_value,
	  "Boot order position", "<position>" },
	{ "attributes", 'a', 0, G_OPTION_ARG_INT, &attributes_value,
	  "Attributes", "<attributes>" },
	{ "description", 'd', 0, G_OPTION_ARG_STRING, &description_value,
	  "Description", "<description>" },
	{ "path", 'p', 0, G_OPTION_ARG_STRING_ARRAY, &paths_value,
	  "Path(s)", "<path>..." },
	{ "data", 'x', 0, G_OPTION_ARG_STRING, &data_value,
	  "Additional data", "<base64 data>" },
	{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet_flag,
	  "Do not show created variable name", NULL },
	{}
};

/** "efibootadd" subcommand */
struct efi_boot_command efibootadd = {
	.description = "- Add EFI boot entry",
	.options = efibootadd_options,
	.exec = efibootadd_exec,
};

/**
 * Delete entry
 *
 * @v argc		Number of remaining command-line arguments
 * @v argv		Remaining command-line arguments
 * @ret ok		Success indicator
 */
static int efibootdel_exec ( int argc, char **argv ) {
	int pos;

	/* Identify entry */
	if ( argc < 2 ) {
		fprintf ( stderr, "Missing argument\n" );
		return 0;
	}
	if ( argc > 2 ) {
		fprintf ( stderr, "Too many arguments\n" );
		return 0;
	}
	pos = parse_id ( argv[1] );
	if ( pos < 0 )
		return 0;

	/* Delete entry */
	if ( ! delete_entry ( pos ) )
		return 0;

	return 1;
}

/** "efibootdel" subcommand options */
static GOptionEntry efibootdel_options[] = {
	{ "type", 't', 0, G_OPTION_ARG_CALLBACK, parse_type,
	  "Load option type", "boot|driver|sysprep" },
	{}
};

/** "efibootdel" subcommand */
struct efi_boot_command efibootdel = {
	.description = "<position>|<name> - Delete EFI boot entry",
	.options = efibootdel_options,
	.exec = efibootdel_exec,
};

/**
 * Invoke subcommand
 *
 * @v argc		Number of command-line arguments
 * @v argv		Command-line arguments
 * @v cmd		Subcommand
 * @ret ok		Success indicator
 */
int efiboot_command ( int argc, char **argv, struct efi_boot_command *cmd ) {
	struct efi_boot_entry **tmp;
	GError *error = NULL;
	GOptionContext *context;
	int i;

	/* Parse command-line options */
	context = g_option_context_new ( cmd->description );
	g_option_context_add_main_entries ( context, cmd->options, NULL );
	if ( ! g_option_context_parse ( context, &argc, &argv, &error ) ) {
		g_printerr ( "Could not parse options: %s\n", error->message );
		goto err_args;
	}

	/* Strip trailing "--" if present.  Not sure why GLib doesn't
	 * do this automatically.
	 */
	if ( ( argc > 1 ) && ( strcmp ( argv[1], "--" ) == 0 ) ) {
		argc--;
		for ( i = 1 ; i < argc ; i++ )
			argv[i] = argv[i + 1];
	}

	/* Get list of boot entries */
	tmp = efiboot_load_all ( type_value );
	if ( ! tmp ) {
		perror ( "No entries found" );
		goto err_load_all;
	}

	/* Create copy of boot entries with space for additional entry */
	entry_count = 0;
	while ( tmp[entry_count] )
		entry_count++;
	entries = malloc ( ( entry_count + 1 /* extra */ + 1 /* NULL */ ) *
			   sizeof ( entries[0] ) );
	if ( ! entries )
		goto err_alloc_entries;
	memcpy ( entries, tmp, ( entry_count * sizeof ( entries[0] ) ) );
	entries[entry_count] = NULL;
	entries[entry_count + 1] = NULL;

	/* Invoke subcommand */
	if ( ! cmd->exec ( argc, argv ) )
		goto err_exec;

	/* Free copy of boot entries */
	free ( entries );

	/* Free original list of boot entries */
	efiboot_free_all ( tmp );

	/* Free option context */
	g_option_context_free ( context );

	return 1;

 err_exec:
	free ( entries );
 err_alloc_entries:
	efiboot_free_all ( tmp );
 err_load_all:
	g_option_context_free ( context );
 err_args:
	return 0;
}
