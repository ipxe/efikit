/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot dump command-line tool
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <efibootdev.h>

/**
 * Main entry point
 *
 * @ret exit		Exit status
 */
int main ( void ) {
	struct efi_boot_entry **entries;
	unsigned int count;
	unsigned int i;
	unsigned int j;

	/* Fetch boot entries */
	entries = efiboot_load_all ( EFIBOOT_TYPE_BOOT );
	if ( ! entries ) {
		fprintf ( stderr, "No boot entries\n" );
		exit ( EXIT_FAILURE );
	}

	/* Dump each entry */
	for ( i = 0 ; entries[i] ; i++ ) {
		printf ( "%s:", efiboot_description ( entries[i] ) );
		count = efiboot_path_count ( entries[i] );
		for ( j = 0 ; j < count ; j++ )
			printf ( " %s", efiboot_path_text ( entries[i], j ) );
		printf ( "\n" );
	}

	/* Free entries */
	efiboot_free_all ( entries );

	exit ( EXIT_SUCCESS );
}
