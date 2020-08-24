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

/** Sample hard disk device path text */
static const char *hddpath_text =
	"PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)";

/** Sample hard disk device path */
static const struct {
	ACPI_HID_DEVICE_PATH pciroot;
	PCI_DEVICE_PATH pci;
	ATAPI_DEVICE_PATH atapi;
	EFI_DEVICE_PATH_PROTOCOL end;
} __attribute__ (( packed )) hddpath = {
	.pciroot = {
		.Header = DPHDR ( ACPI_DEVICE_PATH, ACPI_DP,
				  sizeof ( hddpath.pciroot ) ),
		.HID = HID_PCIROOT,
	},
	.pci = {
		.Header = DPHDR ( HARDWARE_DEVICE_PATH, HW_PCI_DP,
				  sizeof ( hddpath.pci ) ),
		.Function = 0x1,
		.Device = 0x1,
	},
	.atapi = {
		.Header = DPHDR ( MESSAGING_DEVICE_PATH, MSG_ATAPI_DP,
				  sizeof ( hddpath.atapi ) ),
	},
	.end = DPEND,
};

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

/** Sample URI device path text */
static const char *uripath_text =
	"PciRoot(0x0)/Pci(0x1C,0x2)/Pci(0x0,0x1)/MAC(525400AC9C41,0x1)/"
	"IPv4(0.0.0.0)/Uri(http://boot.ipxe.org/ipxe.efi)";

/** Sample URI device path text (unshortened) */
static const char *uripath_text_long =
	"PciRoot(0x0)/Pci(0x1C,0x2)/Pci(0x0,0x1)/MAC(525400AC9C41,0x1)/"
	"IPv4(0.0.0.0,0x0,DHCP,0.0.0.0,0.0.0.0,0.0.0.0)/"
	"Uri(http://boot.ipxe.org/ipxe.efi)";

/** Sample URI device path */
static const struct {
	ACPI_HID_DEVICE_PATH pciroot;
	PCI_DEVICE_PATH pci1;
	PCI_DEVICE_PATH pci2;
	MAC_ADDR_DEVICE_PATH mac;
	IPv4_DEVICE_PATH ipv4;
	URI_DEVICE_PATH uri;
	char uri_text[29];
	EFI_DEVICE_PATH_PROTOCOL end;
} __attribute__ (( packed )) uripath = {
	.pciroot = {
		.Header = DPHDR ( ACPI_DEVICE_PATH, ACPI_DP,
				  sizeof ( uripath.pciroot ) ),
		.HID = HID_PCIROOT,
	},
	.pci1 = {
		.Header = DPHDR ( HARDWARE_DEVICE_PATH, HW_PCI_DP,
				  sizeof ( uripath.pci1 ) ),
		.Function = 0x2,
		.Device = 0x1c,
	},
	.pci2 = {
		.Header = DPHDR ( HARDWARE_DEVICE_PATH, HW_PCI_DP,
				  sizeof ( uripath.pci2 ) ),
		.Function = 0x1,
		.Device = 0x0,
	},
	.mac = {
		.Header = DPHDR ( MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP,
				  sizeof ( uripath.mac ) ),
		.MacAddress = { { 0x52, 0x54, 0x00, 0xac, 0x9c, 0x41, } },
		.IfType = 0x01,
	},
	.ipv4 = {
		.Header = DPHDR ( MESSAGING_DEVICE_PATH, MSG_IPv4_DP,
				  sizeof ( uripath.ipv4 ) ),
	},
	.uri = {
		.Header = DPHDR ( MESSAGING_DEVICE_PATH, MSG_URI_DP,
				  ( sizeof ( uripath.uri ) +
				    sizeof ( uripath.uri_text ) ) ),
	},
	.uri_text = "http://boot.ipxe.org/ipxe.efi",
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

/** Test hard disk device path */
static void test_hddpath ( void **state ) {
	( void ) state;
	assert_to_from_text ( &hddpath.pciroot.Header, true, true,
			      hddpath_text );
}

/** Test MAC device path */
static void test_macpath ( void **state ) {
	( void ) state;
	assert_to_from_text ( &macpath.pciroot.Header, false, false,
			      macpath_text );
}

/** Test URI device path */
static void test_uripath ( void **state ) {
	( void ) state;
	assert_to_from_text ( &uripath.pciroot.Header, true, true,
			      uripath_text );
	assert_to_from_text ( &uripath.pciroot.Header, false, false,
			      uripath_text_long );
}

/** Tests */
static const struct CMUnitTest tests[] = {
	cmocka_unit_test ( test_hddpath ),
	cmocka_unit_test ( test_macpath ),
	cmocka_unit_test ( test_uripath ),
};

/**
 * Main entry point
 *
 * @ret exit		Exit status
 */
int main ( void ) {
	return cmocka_run_group_tests ( tests, NULL, NULL );
}
