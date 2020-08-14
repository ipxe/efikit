/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI device path command-line tool
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <efidevpath.h>

/** Text to convert back to device path */
static char *text = NULL;

/** Use display-only representation */
static gboolean display_only = FALSE;

/** Use shortcut representation" */
static gboolean allow_shortcut = FALSE;

/** Command-line options */
static GOptionEntry options[] = {
	{ "displayonly", 'd', 0, G_OPTION_ARG_NONE, &display_only,
	  "Use display-only representation", NULL },
	{ "shortcuts", 's', 0, G_OPTION_ARG_NONE, &allow_shortcut,
	  "Use shortcut representation", NULL },
	{ "text", 't', 0, G_OPTION_ARG_STRING, &text,
	  "Convert text back to EFI device path", "TEXT" },
	{ NULL }
};

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
	GIOChannel *ioc;
	EFI_DEVICE_PATH_PROTOCOL *path;
	gchar *data;
	gsize len;

	/* Parse command-line options */
	context = g_option_context_new ( " - Convert EFI device paths" );
	g_option_context_add_main_entries ( context, options, NULL );
	if ( ! g_option_context_parse ( context, &argc, &argv, &error ) ) {
		g_printerr ( "Could not parse options: %s\n", error->message );
		exit ( EXIT_FAILURE );
	}
	if ( argc > 1 ) {
		g_printerr ( "Too many arguments\n" );
		exit ( EXIT_FAILURE );
	}

	/* Convert in appropriate direction */
	if ( text ) {

		/* Convert to device path */
		path = efidp_from_text ( text );
		if ( ! path ) {
			g_printerr ( "Could not convert text to path\n" );
			exit ( EXIT_FAILURE );
		}

		/* Output device path */
		fwrite ( path, efidp_len ( path ), 1, stdout );

		/* Free path */
		free ( path );

	} else {

		/* Read device path */
		ioc = g_io_channel_unix_new ( fileno ( stdin ) );
		g_io_channel_set_encoding ( ioc, NULL, &error );
		g_io_channel_read_to_end ( ioc, &data, &len, &error );
		g_io_channel_unref ( ioc );

		/* Check validity */
		if ( ! efidp_valid ( data, len ) ) {
			g_printerr ( "Malformed path" );
			exit ( EXIT_FAILURE );
		}
		path = ( ( EFI_DEVICE_PATH_PROTOCOL * ) data );

		/* Convert to text */
		text = efidp_to_text ( path, display_only, allow_shortcut );
		if ( ! text ) {
			g_printerr ( "Could not convert path to text\n" );
			exit ( EXIT_FAILURE );
		}

		/* Output text */
		printf ( "%s\n", text );

	}

	exit ( EXIT_SUCCESS );
}
