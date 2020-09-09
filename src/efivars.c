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
#include <errno.h>
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

/**
 * Obtain privileges required to access EFI variables
 *
 * @ret ok		Success indicator
 */
static int efivars_raise ( void ) {
	static int raised = 0;
	HANDLE process;
	TOKEN_PRIVILEGES privs;

	/* Do nothing if privileges have already been raised */
	if ( raised )
		return 1;

	/* Look up privilege */
	privs.PrivilegeCount = 1;
	privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if ( ! LookupPrivilegeValue ( NULL, SE_SYSTEM_ENVIRONMENT_NAME,
				      &privs.Privileges[0].Luid ) ) {
		errno = EPERM;
		return 0;
	}

	/* Look up process token */
	if ( ! OpenProcessToken ( GetCurrentProcess(),
				  ( TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY ),
				  &process ) ) {
		errno = EPERM;
		return 0;
	}

	/* Obtain privilege */
	if ( ! AdjustTokenPrivileges ( process, FALSE, &privs, 0, NULL,
				       NULL ) ) {
		errno = EPERM;
		return 0;
	}

	/* Check that privilege was actually assigned */
	if ( GetLastError() != ERROR_SUCCESS ) {
		errno = EPERM;
		return 0;
	}

	/* Record as raised */
	raised = 1;

	return 1;
}

int efivars_read ( const char *name, void **data, size_t *len ) {

	/* Obtain privileges */
	if ( ! efivars_raise() )
		goto err_raise;

	/* Allocate space for variable */
	*data = malloc ( EFIVARS_MAX_LEN );
	if ( ! *data )
		goto err_alloc;

	/* Read variable */
	*len = GetFirmwareEnvironmentVariableA ( name, efivars_global,
						 *data, EFIVARS_MAX_LEN );
	if ( ! *len ) {
		switch ( GetLastError() ) {
		case ERROR_INVALID_FUNCTION:
			errno = ENOSYS;
			break;
		default:
			errno = ENOENT;
			break;
		}
		goto err_read;
	}

	return 1;

 err_read:
	free ( *data );
	*data = NULL;
 err_alloc:
 err_raise:
	return 0;
}

int efivars_write ( const char *name, const void *data, size_t len ) {

	/* Obtain privileges */
	if ( ! efivars_raise() )
		return 0;

	/* Write variable */
	if ( ! SetFirmwareEnvironmentVariableA ( name, efivars_global,
						 ( ( void * ) data ), len ) ) {
		errno = EACCES;
		return 0;
	}

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

#include <errno.h>

int efivars_read ( const char *name, void **data, size_t *len ) {
	( void ) name;
	( void ) data;
	( void ) len;
	errno = ENOTSUP;
	return 0;
}

int efivars_write ( const char *name, const void *data, size_t len ) {
	( void ) name;
	( void ) data;
	( void ) len;
	errno = ENOTSUP;
	return 0;
}

int efivars_exists ( const char *name ) {
	( void ) name;
	return 0;
}

#endif /* EFIVAR_DUMMY */
