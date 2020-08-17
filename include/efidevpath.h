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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <Uefi/UefiBaseType.h>
#include <Protocol/DevicePath.h>

extern bool efidp_valid ( const void *path, size_t max_len );
extern size_t efidp_len ( const EFI_DEVICE_PATH_PROTOCOL *path );
extern EFI_DEVICE_PATH_PROTOCOL * efidp_from_text ( const char *text );
extern char * efidp_to_text ( const EFI_DEVICE_PATH_PROTOCOL *path,
			      bool display_only, bool allow_shortcuts );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _EFIDEVPATH_H */
