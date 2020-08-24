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

extern void test_hddpath ( void **state );
extern void test_macpath ( void **state );
extern void test_uripath ( void **state );

#endif /* _EFIDEVPATHTEST_H */
