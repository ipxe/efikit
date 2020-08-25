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

/** Test sysfs-extracted OVMF "EFI Internal Shell" load option */
void test_shellopt ( void **state ) {
	static const char *paths[1] = {
		"Fv(7CB8BDC9-F8EB-4F34-AAEA-3EE4AF6516A1)/"
		"FvFile(7C04A583-9E3E-4F1C-AD65-E05268D0B4D1)",
	};
	static struct efi_boot_entry_test test = {
		.attributes = LOAD_OPTION_ACTIVE,
		.description = "EFI Internal Shell",
		.paths = paths,
		.count = 1,
	};
	static uint8_t option[] = {
		0x01, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x45, 0x00, 0x46, 0x00,
		0x49, 0x00, 0x20, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x74, 0x00,
		0x65, 0x00, 0x72, 0x00, 0x6e, 0x00, 0x61, 0x00, 0x6c, 0x00,
		0x20, 0x00, 0x53, 0x00, 0x68, 0x00, 0x65, 0x00, 0x6c, 0x00,
		0x6c, 0x00, 0x00, 0x00, 0x04, 0x07, 0x14, 0x00, 0xc9, 0xbd,
		0xb8, 0x7c, 0xeb, 0xf8, 0x34, 0x4f, 0xaa, 0xea, 0x3e, 0xe4,
		0xaf, 0x65, 0x16, 0xa1, 0x04, 0x06, 0x14, 0x00, 0x83, 0xa5,
		0x04, 0x7c, 0x3e, 0x9e, 0x1c, 0x4f, 0xad, 0x65, 0xe0, 0x52,
		0x68, 0xd0, 0xb4, 0xd1, 0x7f, 0xff, 0x04, 0x00
	};

	( void ) state;
	assert_efiboot_option ( &test, ( EFI_LOAD_OPTION * ) option,
				sizeof ( option ) );
}

/** Test sysfs-extracted Fedora load option */
void test_fedoraopt ( void **state ) {
	static const char *paths[1] = {
		"HD(1,GPT,C8F57909-D589-41A1-9958-44C7F229E150,0x800,0x12C000)/"
		"\\EFI\\fedora\\shimx64.efi",
	};
	static struct efi_boot_entry_test test = {
		.attributes = LOAD_OPTION_ACTIVE,
		.description = "Fedora",
		.paths = paths,
		.count = 1,
	};
	static uint8_t option[] = {
		0x01, 0x00, 0x00, 0x00, 0x62, 0x00, 0x46, 0x00, 0x65, 0x00,
		0x64, 0x00, 0x6f, 0x00, 0x72, 0x00, 0x61, 0x00, 0x00, 0x00,
		0x04, 0x01, 0x2a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x08,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x12, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x09, 0x79, 0xf5, 0xc8, 0x89, 0xd5,
		0xa1, 0x41, 0x99, 0x58, 0x44, 0xc7, 0xf2, 0x29, 0xe1, 0x50,
		0x02, 0x02, 0x04, 0x04, 0x34, 0x00, 0x5c, 0x00, 0x45, 0x00,
		0x46, 0x00, 0x49, 0x00, 0x5c, 0x00, 0x66, 0x00, 0x65, 0x00,
		0x64, 0x00, 0x6f, 0x00, 0x72, 0x00, 0x61, 0x00, 0x5c, 0x00,
		0x73, 0x00, 0x68, 0x00, 0x69, 0x00, 0x6d, 0x00, 0x78, 0x00,
		0x36, 0x00, 0x34, 0x00, 0x2e, 0x00, 0x65, 0x00, 0x66, 0x00,
		0x69, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x04, 0x00
	};

	( void ) state;
	assert_efiboot_option ( &test, ( EFI_LOAD_OPTION * ) option,
				sizeof ( option ) );
}
