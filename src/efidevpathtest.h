/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI device path library self-tests
 *
 */

#ifndef _EFIDEVPATHTEST_H
#define _EFIDEVPATHTEST_H

#include <efidevpath.h>

extern void assert_efidp_to_text ( const EFI_DEVICE_PATH_PROTOCOL *path,
				   bool display_only, bool allow_shortcuts,
				   const char *expected );
extern void assert_efidp_from_text ( const char *text,
				     const EFI_DEVICE_PATH_PROTOCOL *expected );
extern void assert_efidp_text ( const EFI_DEVICE_PATH_PROTOCOL *path,
				bool display_only, bool allow_shortcuts,
				const char *text );
extern void test_hddpath ( void **state );
extern void test_macpath ( void **state );
extern void test_uripath ( void **state );

#endif /* _EFIDEVPATHTEST_H */
