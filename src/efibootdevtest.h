/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot device selection library self-tests
 *
 */

#ifndef _EFIBOOTDEVTEST_H
#define _EFIBOOTDEVTEST_H

extern void test_hddopt ( void **state );
extern void test_badopt ( void **state );

#endif /* _EFIBOOTDEVTEST_H */