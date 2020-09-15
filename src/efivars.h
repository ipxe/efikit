/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Conversion between UTF-8 and EFI UCS2-LE strings
 *
 */

#ifndef _EFIVARS_H
#define _EFIVARS_H

#include <stddef.h>

extern int efivars_read ( const char *name, void **data, size_t *len );
extern int efivars_write ( const char *name, const void *data, size_t len );
extern int efivars_delete ( const char *name );
extern int efivars_exists ( const char *name );

#endif /* _EFIVARS_H */
