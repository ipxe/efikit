/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot dump command-line tool
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <efibootdev.h>

/** Show variable name */
static gboolean show_name = FALSE;

/** Show attributes */
static gboolean show_attributes = FALSE;

/** Show description */
static gboolean show_description = FALSE;

/** Show first path */
static gboolean show_path = FALSE;

/** Show all paths */
static gboolean show_paths = FALSE;

/** Show additional data */
static gboolean show_data = FALSE;

/** Command-line options */
static GOptionEntry options[] = {
	{ "name", 'n', 0, G_OPTION_ARG_NONE, &show_name,
	  "Show variable name", NULL },
	{ "attributes", 'a', 0, G_OPTION_ARG_NONE, &show_attributes,
	  "Show attributes", NULL },
	{ "description", 'd', 0, G_OPTION_ARG_NONE, &show_description,
	  "Show description", NULL },
	{ "path", 'p', 0, G_OPTION_ARG_NONE, &show_path,
	  "Show primary path", NULL },
	{ "paths", 'P', 0, G_OPTION_ARG_NONE, &show_paths,
	  "Show all paths", NULL },
	{ "data", 'x', 0, G_OPTION_ARG_NONE, &show_data,
	  "Show additional data", NULL },
	{}
};

/**
 * Show boot entry
 *
 * @v entry		Boot entry
 */
static void show_entry ( struct efi_boot_entry *entry ) {
	const char *sep = "";
	char *encoded;
	bool show_all;
	unsigned int count;
	unsigned int i;

	/* Show all fields if no fields are specified */
	show_all = ( ! ( show_name || show_attributes || show_description ||
			 show_path || show_paths || show_data ) );

	/* Show variable name, if applicable */
	if ( show_all || show_name ) {
		printf ( "%s%s", sep, efiboot_name ( entry ) );
		sep = " ";
	}

	/* Show attributes, if applicable */
	if ( show_all || show_attributes ) {
		printf ( "%s%08x", sep, efiboot_attributes ( entry ) );
		sep = " ";
	}

	/* Show description, if applicable */
	if ( show_all || show_description ) {
		printf ( "%s%s", sep, efiboot_description ( entry ) );
		sep = " ";
	}

	/* Show path(s), if applicable */
	count = ( ( show_all || show_paths ) ? efiboot_path_count ( entry ) :
		  show_path ? 1 : 0 );
	for ( i = 0 ; i < count ; i++ ) {
		printf ( "%s%s", sep, efiboot_path_text ( entry, i ) );
		sep = " ";
	}

	/* Show additional data, if applicable */
	if ( ( show_all || show_data ) && efiboot_data_len ( entry ) ) {
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
 * Main entry point
 *
 * @v argc		Number of command-line arguments
 * @v argv		Command-line arguments
 * @ret exit		Exit status
 */
int main ( int argc, char **argv ) {
	GError *error = NULL;
	GOptionContext *context;
	struct efi_boot_entry **entries;
	unsigned int i;

	/* Parse command-line options */
	context = g_option_context_new ( " - Dump EFI boot devices" );
	g_option_context_add_main_entries ( context, options, NULL );
	if ( ! g_option_context_parse ( context, &argc, &argv, &error ) ) {
		g_printerr ( "Could not parse options: %s\n", error->message );
		exit ( EXIT_FAILURE );
	}
	if ( argc > 1 ) {
		g_printerr ( "Too many arguments\n" );
		exit ( EXIT_FAILURE );
	}

	/* Fetch boot entries */
	entries = efiboot_load_all ( EFIBOOT_TYPE_BOOT );
	if ( ! entries ) {
		perror ( "No boot entries" );
		exit ( EXIT_FAILURE );
	}

	/* Dump each entry */
	for ( i = 0 ; entries[i] ; i++ )
		show_entry ( entries[i] );

	/* Free entries */
	efiboot_free_all ( entries );

	exit ( EXIT_SUCCESS );
}
