/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI device path library
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <Uefi/UefiBaseType.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DebugPort.h>
#include <Library/DevicePathLib.h>
#include <efidevpath.h>

#include "strconvert.h"
#include "edk2/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.h"

/* UART device path GUID (missing from EDK2 headers) */
#define EFI_UART_DEVICE_PATH_GUID {					\
		0x37499a9d, 0x542f, 0x4c89,				\
		{ 0xa0, 0x26, 0x35, 0xda, 0x14, 0x20, 0x94, 0xe4 }	\
	}

/* Assorted GUIDs used by UefiDevicePathLib */
EFI_GUID gEfiDebugPortProtocolGuid = DEVICE_PATH_MESSAGING_DEBUGPORT;
EFI_GUID gEfiPcAnsiGuid = EFI_PC_ANSI_GUID;
EFI_GUID gEfiVT100Guid = EFI_VT_100_GUID;
EFI_GUID gEfiVT100PlusGuid = EFI_VT_100_PLUS_GUID;
EFI_GUID gEfiVTUTF8Guid = EFI_VT_UTF8_GUID;
EFI_GUID gEfiUartDevicePathGuid = EFI_UART_DEVICE_PATH_GUID;
EFI_GUID gEfiSasDevicePathGuid = EFI_SAS_DEVICE_PATH_GUID;
EFI_GUID gEfiVirtualDiskGuid = EFI_VIRTUAL_DISK_GUID;
EFI_GUID gEfiVirtualCdGuid = EFI_VIRTUAL_CD_GUID;
EFI_GUID gEfiPersistentVirtualDiskGuid = EFI_PERSISTENT_VIRTUAL_DISK_GUID;
EFI_GUID gEfiPersistentVirtualCdGuid = EFI_PERSISTENT_VIRTUAL_CD_GUID;

/**
 * Check validity of device path
 *
 * @v path		Potential EFI device path
 * @v max_len		Maximum length of device path (or 0 to ignore length)
 * @ret valid		Device path is valid
 */
bool efidp_valid ( const void *path, size_t max_len ) {

	/* Check validity */
	if ( ! IsDevicePathValid ( path, max_len ) ) {
		errno = EINVAL;
		return false;
	}

	return true;
}

/**
 * Check plausibility of device path
 *
 * @v path		EFI device path
 * @ret plausible	Device path is plausible
 *
 * When constructing a device path from a textual representation, EFI
 * will treat any unrecognised string as a filename.  This can lead to
 * unexpected behaviour.  For example, if a device path component
 * Uri() is mistyped as URI(), it will become a FILEPATH_DEVICE_PATH
 * with the filename "URI()", rather than becoming a URI_DEVICE_PATH.
 *
 * It is implausible that any real FILEPATH_DEVICE_PATH would be of
 * the form "Xxx(...)".  This function checks for such plausibility.
 */
bool efidp_plausible ( const EFI_DEVICE_PATH_PROTOCOL *path ) {
	FILEPATH_DEVICE_PATH *filepath;
	CHAR16 *filename;
	unsigned int remaining;

	/* Iterate over device path nodes */
	for ( ; ! IsDevicePathEndType ( path ) ;
	      path = NextDevicePathNode ( path ) ) {

		/* Ignore everything other than FILEPATH_DEVICE_PATH */
		if ( DevicePathType ( path ) != MEDIA_DEVICE_PATH )
			continue;
		if ( DevicePathSubType ( path ) != MEDIA_FILEPATH_DP )
			continue;

		/* Extract filename */
		filepath = ( ( FILEPATH_DEVICE_PATH * ) path );
		filename = filepath->PathName;
		remaining = ( ( DevicePathNodeLength ( path ) -
				SIZE_OF_FILEPATH_DEVICE_PATH ) /
			      sizeof ( filename[0] ) );

		/* Trim trailing NUL (if present) */
		if ( remaining && ( filename[remaining - 1] == L'\0' ) )
			remaining--;

		/* Trim initial alphanumeric characters */
		while ( remaining && isalnum ( *filename ) ) {
			filename++;
			remaining--;
		}

		/* Treat as implausible if remaining portion matches "(...)" */
		if ( remaining && ( filename[0] == L'(' ) &&
		     ( filename[remaining - 1] == L')' ) ) {
			errno = EINVAL;
			return false;
		}
	}

	return true;
}

/**
 * Get length of device path
 *
 * @v path		EFI device path
 * @ret len		Length of device path in bytes (including terminator)
 */
size_t efidp_len ( const EFI_DEVICE_PATH_PROTOCOL *path ) {
	return UefiDevicePathLibGetDevicePathSize ( path );
}

/**
 * Construct device path from textual representation
 *
 * @v text		Textual representation (in UTF-8)
 * @v allow_implausible	Allow implausible device paths
 * @ret path		EFI device path, or NULL on error
 *
 * The device path is allocated using malloc() and must eventually be
 * freed by the caller.
 */
EFI_DEVICE_PATH_PROTOCOL * efidp_from_text ( const char *text,
					     bool allow_implausible ) {
	CHAR16 *efitext;
	EFI_DEVICE_PATH_PROTOCOL *efidp;

	/* Convert to EFI string */
	efitext = utf8_to_efi ( text );
	if ( ! efitext )
		goto err_efitext;

	/* Convert to EFI device path */
	efidp = UefiDevicePathLibConvertTextToDevicePath ( efitext );
	if ( ! efidp ) {
		errno = EINVAL;
		goto err_efidp;
	}

	/* Check for plausibility */
	if ( ! ( allow_implausible || efidp_plausible ( efidp ) ) )
		goto err_implausible;

	/* Free EFI string */
	free ( efitext );

	return efidp;

 err_implausible:
	free ( efidp );
 err_efidp:
	free ( efitext );
 err_efitext:
	return NULL;
}

/**
 * Get textual representation of device path
 *
 * @v path		EFI device path
 * @v display_only	Use shorter text representation of the display node
 * @v allow_shortcuts	Use shortcut forms of text representation
 * @ret text		Textual representation
 *
 * The textual representation is allocated using malloc() and must
 * eventually be freed by the caller.
 *
 * The UEFI specification is remarkably vague on the difference
 * between @c display_only and @c allow_shortcuts.
 */
char * efidp_to_text ( const EFI_DEVICE_PATH_PROTOCOL *path, bool display_only,
		       bool allow_shortcuts ) {
	CHAR16 *efitext;
	char *text;

	/* Convert to EFI string */
	efitext = UefiDevicePathLibConvertDevicePathToText ( path, display_only,
							     allow_shortcuts );
	if ( ! efitext ) {
		errno = EINVAL;
		goto err_efitext;
	}

	/* Convert to UTF8 string */
	text = efi_to_utf8 ( efitext );
	if ( ! text )
		goto err_text;

	/* Free EFI string */
	free ( efitext );

	return text;

	free ( text );
 err_text:
	free ( efitext );
 err_efitext:
	return NULL;
}
