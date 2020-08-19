/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI device path library self-tests
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cmocka.h>
#include <efidevpath.h>

/** Construct device path protocol header */
#define DPHDR( type, subtype, length ) {				\
		.Type = (type),						\
		.SubType = (subtype),					\
		.Length = { (length) & 0xff, (length) >> 8 },		\
	}

/** End device path */
#define DPEND DPHDR ( END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, \
		      sizeof ( EFI_DEVICE_PATH_PROTOCOL ) )

/** PCI root ACPI HID */
#define HID_PCIROOT EISA_PNP_ID ( 0x0a03 )

/** Sample MAC device path text */
static const char *macpath_text =
	"PciRoot(0x0)/Pci(0x3,0x0)/MAC(525400123456,0x1)";

/** Sample MAC device path */
static const struct {
	ACPI_HID_DEVICE_PATH pciroot;
	PCI_DEVICE_PATH pci;
	MAC_ADDR_DEVICE_PATH mac;
	EFI_DEVICE_PATH_PROTOCOL end;
} __attribute__ (( packed )) macpath = {
	.pciroot = {
		.Header = DPHDR ( ACPI_DEVICE_PATH, ACPI_DP,
				  sizeof ( macpath.pciroot ) ),
		.HID = HID_PCIROOT,
	},
	.pci = {
		.Header = DPHDR ( HARDWARE_DEVICE_PATH, HW_PCI_DP,
				  sizeof ( macpath.pci ) ),
		.Function = 0x0,
		.Device = 0x3,
	},
	.mac = {
		.Header = DPHDR ( MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP,
				  sizeof ( macpath.mac ) ),
		.MacAddress = { { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56, } },
		.IfType = 0x01,
	},
	.end = DPEND,
};

/**
 * Test conversion from device path to text
 *
 * @v path		Device path
 * @v display_only	Use shorter text representation of the node
 * @v allow_shortcuts	Use shortcut forms of text representation
 * @v expected		Expected text
 */
static void assert_to_text ( const EFI_DEVICE_PATH_PROTOCOL *path,
			     bool display_only, bool allow_shortcuts,
			     const char *expected ) {
	char *text;

	/* Convert device path to text */
	text = efidp_to_text ( path, display_only, allow_shortcuts );
	assert_non_null ( text );

	/* Check text */
	assert_string_equal ( text, expected );

	/* Free text */
	free ( text );
}

/**
 * Test conversion from text to device path
 *
 * @v text		Text
 * @v expected		Expected device path
 */
static void assert_from_text ( const char *text,
			       const EFI_DEVICE_PATH_PROTOCOL *expected ) {
	EFI_DEVICE_PATH_PROTOCOL *path;
	size_t len;

	/* Convert text to device path */
	path = efidp_from_text ( text );
	assert_non_null ( path );

	/* Check length */
	len = efidp_len ( path );
	assert_int_equal ( len, efidp_len ( expected ) );

	/* Check path */
	assert_memory_equal ( path, expected, len );

	/* Free path */
	free ( path );
}

/**
 * Test conversion from device path to text and back
 *
 * @v path		Device path
 * @v display_only	Use shorter text representation of the node
 * @v allow_shortcuts	Use shortcut forms of text representation
 * @v text		Device path text
 */
static void assert_to_from_text ( const EFI_DEVICE_PATH_PROTOCOL *path,
				  bool display_only, bool allow_shortcuts,
				  const char *text ) {
	assert_to_text ( path, display_only, allow_shortcuts, text );
	assert_from_text ( text, path );
}

/** Test MAC device path */
static void test_macpath ( void **state ) {
	( void ) state;
	assert_to_from_text ( &macpath.pciroot.Header, false, false,
			      macpath_text );
}

/** Tests */
static const struct CMUnitTest tests[] = {
	cmocka_unit_test ( test_macpath ),
};

/**
 * Main entry point
 *
 * @ret exit		Exit status
 */
int main ( void ) {
	return cmocka_run_group_tests ( tests, NULL, NULL );
}