/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI variable access
 *
 */

#include <stddef.h>
#include "efivars.h"
#include "config.h"

/**
 * Read global variable
 *
 * @v name		Variable name
 * @v data		Data pointer to fill in
 * @v len		Length to fill in
 * @ret ok		Success indicator
 *
 * The data storage is allocated using malloc() and must eventually be
 * freed by the caller.
 */
int efivars_read ( const char *name, void **data, size_t *len );

/**
 * Write global variable
 *
 * @v name		Variable name
 * @v data		Data
 * @v len		Length of data
 * @ret ok		Success indicator
 */
int efivars_write ( const char *name, const void *data, size_t len );

/**
 * Check for existence of global variable
 *
 * @v name		Variable name
 * @ret exists		Variable exists
 */
int efivars_exists ( const char *name );

/*****************************************************************************
 *
 * Linux: via libefivar
 *
 ****************************************************************************
 */

#ifdef EFIVAR_LIBEFIVAR

#include <efivar.h>
#include <sys/types.h>

int efivars_read ( const char *name, void **data, size_t *len ) {
	uint32_t attributes;

	/* Read variable */
	if ( efi_get_variable ( EFI_GLOBAL_GUID, name, ( ( uint8_t ** ) data ),
				len, &attributes ) != 0 )
		return 0;

	return 1;
}

int efivars_write ( const char *name, const void *data, size_t len ) {

	/* Write variable */
	if ( efi_set_variable ( EFI_GLOBAL_GUID, name, ( ( void * ) data ), len,
				( EFI_VARIABLE_NON_VOLATILE |
				  EFI_VARIABLE_BOOTSERVICE_ACCESS |
				  EFI_VARIABLE_RUNTIME_ACCESS ),
				( S_IRUSR | S_IWUSR |
				  S_IRGRP | S_IROTH ) ) != 0 )
		return 0;

	return 1;
}

int efivars_exists ( const char *name ) {
	size_t len;

	/* Check existence */
	if ( efi_get_variable_size ( EFI_GLOBAL_GUID, name, &len ) != 0 )
		return 0;

	return 1;
}

#endif /* EFIVAR_LIBEFIVAR */

/*****************************************************************************
 *
 * Windows: via GetFirmwareEnvironmentVariable et al
 *
 ****************************************************************************
 */

#ifdef EFIVAR_WINDOWS

#include <stdlib.h>
#include <windows.h>

/** Global variable GUID */
static const char efivars_global[] =
	"{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}";

/** Maximum length of variable data
 *
 * The Windows API seems to provide no way to get the length of the
 * variable other than attempting to fetch it.
 */
#define EFIVARS_MAX_LEN 4096

int efivars_read ( const char *name, void **data, size_t *len ) {

	/* Allocate space for variable */
	*data = malloc ( EFIVARS_MAX_LEN );
	if ( ! *data )
		goto err_alloc;

	/* Read variable */
	*len = GetFirmwareEnvironmentVariableA ( name, efivars_global,
						 *data, EFIVARS_MAX_LEN );
	if ( ! *len )
		goto err_read;

	return 1;

 err_read:
	free ( *data );
	*data = NULL;
 err_alloc:
	return 0;
}

int efivars_write ( const char *name, const void *data, size_t len ) {

	/* Write variable */
	if ( ! SetFirmwareEnvironmentVariableA ( name, efivars_global,
						 ( ( void * ) data ), len ) )
		return 0;

	return 1;
}

int efivars_exists ( const char *name ) {
	void *data;
	size_t len;

	/* Attempt to read variable */
	if ( ! efivars_read ( name, &data, &len ) )
		return 0;

	/* Free variable */
	free ( data );

	return 1;
}

#endif /* EFIVAR_WINDOWS */

/*****************************************************************************
 *
 * Other: dummy API that always fails
 *
 ****************************************************************************
 */

#ifdef EFIVAR_DUMMY

int efivars_read ( const char *name, void **data, size_t *len ) {
	( void ) name;
	( void ) data;
	( void ) len;
	return 0;
}

int efivars_write ( const char *name, const void *data, size_t len ) {
	( void ) name;
	( void ) data;
	( void ) len;
	return 0;
}

int efivars_exists ( const char *name ) {
	( void ) name;
	return 0;
}

#endif /* EFIVAR_DUMMY */
