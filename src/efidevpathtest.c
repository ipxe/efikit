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

#include "efidevpathtest.h"

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

/** Test hard disk device path */
void test_hddpath ( void **state ) {
	static const char *text = "PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)";
	static const struct {
		ACPI_HID_DEVICE_PATH pciroot;
		PCI_DEVICE_PATH pci;
		ATAPI_DEVICE_PATH atapi;
		EFI_DEVICE_PATH_PROTOCOL end;
	} __attribute__ (( packed )) path = {
		.pciroot = EFIDP_PCIROOT ( 0x0 ),
		.pci = EFIDP_PCI ( 0x01, 0x1 ),
		.atapi = EFIDP_ATA ( 0, 0, 0 ),
		.end = EFIDP_END,
	};

	( void ) state;
	assert_to_from_text ( &path.pciroot.Header, true, true, text );
}

/** Test MAC device path */
void test_macpath ( void **state ) {
	static const char *text =
		"PciRoot(0x0)/Pci(0x3,0x0)/MAC(525400123456,0x1)";
	static const struct {
		ACPI_HID_DEVICE_PATH pciroot;
		PCI_DEVICE_PATH pci;
		MAC_ADDR_DEVICE_PATH mac;
		EFI_DEVICE_PATH_PROTOCOL end;
	} __attribute__ (( packed )) path = {
		.pciroot = EFIDP_PCIROOT ( 0 ),
		.pci = EFIDP_PCI ( 0x03, 0x0 ),
		.mac = EFIDP_MAC ( ( 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 ), 1 ),
		.end = EFIDP_END,
	};

	( void ) state;
	assert_to_from_text ( &path.pciroot.Header, false, false, text );
}

/** Test URI device path */
void test_uripath ( void **state ) {
	static const char *text =
		"PciRoot(0x0)/Pci(0x1C,0x2)/Pci(0x0,0x1)/MAC(525400AC9C41,0x1)/"
		"IPv4(0.0.0.0)/Uri(http://boot.ipxe.org/ipxe.efi)";
	static const char *text_long =
		"PciRoot(0x0)/Pci(0x1C,0x2)/Pci(0x0,0x1)/MAC(525400AC9C41,0x1)/"
		"IPv4(0.0.0.0,0x0,DHCP,0.0.0.0,0.0.0.0,0.0.0.0)/"
		"Uri(http://boot.ipxe.org/ipxe.efi)";
	static const struct {
		ACPI_HID_DEVICE_PATH pciroot;
		PCI_DEVICE_PATH pci1;
		PCI_DEVICE_PATH pci2;
		MAC_ADDR_DEVICE_PATH mac;
		IPv4_DEVICE_PATH ipv4;
		URI_DEVICE_PATH uri;
		char uri_text[29];
		EFI_DEVICE_PATH_PROTOCOL end;
	} __attribute__ (( packed )) path = {
		.pciroot = EFIDP_PCIROOT ( 0 ),
		.pci1 = EFIDP_PCI ( 0x1c, 0x2 ),
		.pci2 = EFIDP_PCI ( 0x00, 0x1 ),
		.mac = EFIDP_MAC ( ( 0x52, 0x54, 0x00, 0xac, 0x9c, 0x41 ), 1 ),
		.ipv4 = EFIDP_IPv4_AUTO,
		.uri = EFIDP_URI_HDR ( sizeof ( path.uri_text ) ),
		.uri_text = "http://boot.ipxe.org/ipxe.efi",
		.end = EFIDP_END,
	};

	( void ) state;
	assert_to_from_text ( &path.pciroot.Header, true, true, text );
	assert_to_from_text ( &path.pciroot.Header, false, false, text_long );
}
