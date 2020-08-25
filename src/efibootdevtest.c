/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot device selection library self-tests
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>
#include <efibootdev.h>

#include "efidevpathtest.h"
#include "efibootdevtest.h"

/** An EFI boot entry test point */
struct efi_boot_entry_test {
	/** Attributes */
	uint32_t attributes;
	/** Description (as UTF8 string) */
	const char *description;
	/** Device paths (as UTF8 strings) */
	const char **paths;
	/** Number of device paths */
	unsigned int count;
	/** Optional data */
	const void *data;
	/** Length of optional data */
	size_t len;
};

/**
 * Test equality of EFI boot entries
 *
 * @v entry		EFI boot entry
 * @v test		Test point
 */
static void assert_efiboot_entry ( const struct efi_boot_entry *entry,
				   const struct efi_boot_entry_test *test ) {
	const EFI_DEVICE_PATH_PROTOCOL *path;
	unsigned int i;

	assert_int_equal ( efiboot_attributes ( entry ), test->attributes );
	assert_string_equal ( efiboot_description ( entry ),
			      test->description );
	assert_int_equal ( efiboot_path_count ( entry ), test->count );
	for ( i = 0 ; i < test->count ; i++ ) {
		path = efiboot_path ( entry, i );
		assert_non_null ( path );
		assert_efidp_from_text ( test->paths[i], path );
	}
	assert_int_equal ( efiboot_data_len ( entry ), test->len );
	assert_memory_equal ( efiboot_data ( entry ), test->data, test->len );
}

/**
 * Test construction of EFI load option
 *
 * @v test		Test point
 * @v expected		Expected EFI load option
 * @v expected_len	Length of expected EFI load option
 */
static void assert_efiboot_to_option ( const struct efi_boot_entry_test *test,
				       const EFI_LOAD_OPTION *expected,
				       size_t expected_len ) {
	EFI_DEVICE_PATH_PROTOCOL *paths[test->count];
	struct efi_boot_entry *entry;
	EFI_LOAD_OPTION *option;
	size_t len;
	unsigned int i;

	/* Construct device paths */
	for ( i = 0 ; i < test->count ; i++ ) {
		paths[i] = efidp_from_text ( test->paths[i] );
		assert_non_null ( paths[i] );
	}

	/* Construct boot entry */
	entry = efiboot_new ( test->attributes, test->description, paths,
			      test->count, test->data, test->len );
	assert_non_null ( entry );
	assert_efiboot_entry ( entry, test );

	/* Construct load option */
	option = efiboot_to_option ( entry, &len );
	assert_non_null ( option );
	assert_int_equal ( len, expected_len );
	assert_memory_equal ( expected, option, len );

	/* Free load option */
	free ( option );

	/* Free boot entry */
	efiboot_free ( entry );

	/* Free device paths */
	for ( i = 0 ; i < test->count ; i++ )
		free ( paths[i] );
}

/**
 * Test parsing of EFI load option
 *
 * @v option		EFI load option
 * @v len		Length of EFI load option
 * @v expected		Expected test point
 */
static void
assert_efiboot_from_option ( const EFI_LOAD_OPTION *option, size_t len,
			     const struct efi_boot_entry_test *test ) {
	struct efi_boot_entry *entry;

	/* Parse load option */
	entry = efiboot_from_option ( option, len );
	assert_non_null ( entry );

	/* Check boot entry */
	assert_efiboot_entry ( entry, test );

	/* Free boot entry */
	efiboot_free ( entry );
}

/**
 * Test failure to parse EFI load option
 *
 * @v option		EFI load option
 * @v len		Length of EFI load option
 */
static void assert_efiboot_from_option_fail ( const EFI_LOAD_OPTION *option,
					      size_t len ) {
	assert_null ( efiboot_from_option ( option, len ) );
}

/**
 * Test construction and parsing of EFI load option
 *
 * @v test		Test point
 * @v option		EFI load option
 * @v len		Length of EFI load option
 */
static void assert_efiboot_option ( const struct efi_boot_entry_test *test,
				    const EFI_LOAD_OPTION *option,
				    size_t len ) {
	assert_efiboot_to_option ( test, option, len );
	assert_efiboot_from_option ( option, len, test );
}

/** Test hard disk load option */
void test_hddopt ( void **state ) {
	static const char *paths[1] = { "PciRoot(0x0)/Pci(0x1,0x2)/Ata(0x0)" };
	static const struct efi_boot_entry_test test = {
		.attributes = LOAD_OPTION_ACTIVE,
		.description = "Hard disk",
		.paths = paths,
		.count = 1,
	};
	static const struct {
		EFI_LOAD_OPTION option;
		CHAR16 description[10];
		struct {
			ACPI_HID_DEVICE_PATH pciroot;
			PCI_DEVICE_PATH pci;
			ATAPI_DEVICE_PATH atapi;
			EFI_DEVICE_PATH_PROTOCOL end;
		} __attribute__ (( packed )) path0;
	} __attribute__ (( packed )) option = {
		.option = {
			.Attributes = LOAD_OPTION_ACTIVE,
			.FilePathListLength = sizeof ( option.path0 ),
		},
		.description = L"Hard disk",
		.path0 = {
			.pciroot = EFIDP_PCIROOT ( 0x0 ),
			.pci = EFIDP_PCI ( 0x01, 0x2 ),
			.atapi = EFIDP_ATA ( 0, 0, 0 ),
			.end = EFIDP_END,
		},
	};

	( void ) state;
	assert_efiboot_option ( &test, &option.option, sizeof ( option ) );
}

/** Test malformed options */
void test_badopt ( void **state ) {
	static const char *paths[1] = { "PciRoot(0x0)/Pci(0x1,0x2)/Ata(0x0)" };
	static const uint8_t data[5] = { 1, 2, 3, 4, 5 };
	static struct efi_boot_entry_test test = {
		.attributes = LOAD_OPTION_ACTIVE,
		.description = "Bad option",
		.paths = paths,
		.count = 1,
		.data = data,
		.len = sizeof ( data ),
	};
	static struct {
		EFI_LOAD_OPTION option;
		CHAR16 description[11];
		struct {
			ACPI_HID_DEVICE_PATH pciroot;
			PCI_DEVICE_PATH pci;
			ATAPI_DEVICE_PATH atapi;
			EFI_DEVICE_PATH_PROTOCOL end;
		} __attribute__ (( packed )) path0;
		uint8_t data[5];
	} __attribute__ (( packed )) option = {
		.option = {
			.Attributes = LOAD_OPTION_ACTIVE,
			.FilePathListLength = sizeof ( option.path0 ),
		},
		.description = L"Bad option",
		.path0 = {
			.pciroot = EFIDP_PCIROOT ( 0x0 ),
			.pci = EFIDP_PCI ( 0x01, 0x2 ),
			.atapi = EFIDP_ATA ( 0, 0, 0 ),
			.end = EFIDP_END,
		},
		.data = { 1, 2, 3, 4, 5 },
	};
	unsigned int i;

	( void ) state;

	/* Check that initial structure is OK */
	assert_efiboot_option ( &test, &option.option, sizeof ( option ) );

	/* Check that malformed FilePathListLength fails to parse */
	option.option.FilePathListLength = ( sizeof ( option.path0 ) - 1 );
	assert_efiboot_from_option_fail ( &option.option, sizeof ( option ) );
	option.option.FilePathListLength = ( sizeof ( option.path0 ) + 1 );
	assert_efiboot_from_option_fail ( &option.option, sizeof ( option ) );
	option.option.FilePathListLength = sizeof ( option );
	assert_efiboot_from_option_fail ( &option.option, sizeof ( option ) );
	option.option.FilePathListLength = 0;
	assert_efiboot_from_option_fail ( &option.option, sizeof ( option ) );
	option.option.FilePathListLength = sizeof ( option.path0 );
	assert_efiboot_option ( &test, &option.option, sizeof ( option ) );

	/* Check that unterminated description fails to parse */
	option.description[10] = L'x';
	assert_efiboot_from_option_fail ( &option.option, sizeof ( option ) );
	option.description[10] = L'\0';
	assert_efiboot_option ( &test, &option.option, sizeof ( option ) );

	/* Check that malformed device path fails to parse */
	option.path0.end.Length[0] = 1;
	assert_efiboot_from_option_fail ( &option.option, sizeof ( option ) );
	option.path0.end.Length[0] = 0;
	assert_efiboot_from_option_fail ( &option.option, sizeof ( option ) );
	option.path0.end.Length[0] = sizeof ( option.path0.end );
	assert_efiboot_option ( &test, &option.option, sizeof ( option ) );

	/* Check that truncated length fails to parse */
	for ( i = 1 ; i < sizeof ( data ) ; i++ ) {
		test.len = ( sizeof ( data ) - i );
		assert_efiboot_option ( &test, &option.option,
					( sizeof ( option ) - i ) );
	}
	assert_efiboot_from_option_fail ( &option.option,
					  ( sizeof ( option ) -
					    sizeof ( data ) - 1 ) );
	test.len = sizeof ( data );
	assert_efiboot_option ( &test, &option.option, sizeof ( option ) );
}
