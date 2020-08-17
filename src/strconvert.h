/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Conversion between UTF-8 and EFI UCS2-LE strings
 *
 */

#ifndef _STRCONVERT_H
#define _STRCONVERT_H

#include <Uefi/UefiBaseType.h>

extern CHAR16 * utf8_to_efi ( const char *utf8 );
extern char * efi_to_utf8 ( const CHAR16 *efi );

#endif /* _STRCONVERT_H */
