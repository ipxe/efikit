/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI kit library self-tests
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cmocka.h>

#include "memalloctest.h"
#include "efidevpathtest.h"
#include "efibootdevtest.h"

/** Tests */
static const struct CMUnitTest tests[] = {
	cmocka_unit_test ( test_memalloc ),
	cmocka_unit_test ( test_hddpath ),
	cmocka_unit_test ( test_macpath ),
	cmocka_unit_test ( test_uripath ),
	cmocka_unit_test ( test_fvfilepath ),
	cmocka_unit_test ( test_hddfilepath ),
	cmocka_unit_test ( test_implausiblepath ),
	cmocka_unit_test ( test_hddopt ),
	cmocka_unit_test ( test_badopt ),
	cmocka_unit_test ( test_shellopt ),
	cmocka_unit_test ( test_fedoraopt ),
	cmocka_unit_test ( test_typename ),
	cmocka_unit_test ( test_varname ),
};

/**
 * Main entry point
 *
 * @ret exit		Exit status
 */
int main ( void ) {
	return cmocka_run_group_tests ( tests, NULL, NULL );
}
