/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Modify EFI boot device command-line tool
 *
 */

#include <stdlib.h>
#include "efibootcli.h"

/**
 * Main entry point
 *
 * @v argc		Number of command-line arguments
 * @v argv		Command-line arguments
 * @ret exit		Exit status
 */
int main ( int argc, char **argv ) {

	if ( ! efiboot_command ( argc, argv, &efibootmod ) )
		exit ( EXIT_FAILURE );

	exit ( EXIT_SUCCESS );
}
