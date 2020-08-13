/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI device path library
 *
 */

#ifndef _EFIDEVPATH_H
#define _EFIDEVPATH_H

#include <stdbool.h>
#include <Uefi/UefiBaseType.h>
#include <Protocol/DevicePath.h>

extern bool efidp_valid ( void *path, size_t max_len );
extern size_t efidp_len ( EFI_DEVICE_PATH_PROTOCOL *path );
extern EFI_DEVICE_PATH_PROTOCOL * efidp_from_text ( char *text );
extern char * efidp_to_text ( EFI_DEVICE_PATH_PROTOCOL *path, bool display_only,
			      bool allow_shortcuts );

#endif /* _EFIDEVPATH_H */
