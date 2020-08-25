/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI device path library
 *
 */

#ifndef _EFIDEVPATH_H
#define _EFIDEVPATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <Uefi/UefiBaseType.h>
#include <Protocol/DevicePath.h>

/** Construct device path protocol header */
#define EFIDP_HDR( type, subtype, length ) {				\
	.Type = (type),							\
	.SubType = (subtype),						\
	.Length = { (length) & 0xff, (length) >> 8 },			\
	}

/** End device path */
#define EFIDP_END EFIDP_HDR ( END_DEVICE_PATH_TYPE,			\
			      END_ENTIRE_DEVICE_PATH_SUBTYPE,		\
			      sizeof ( EFI_DEVICE_PATH_PROTOCOL ) )

/** PCI root ACPI HID */
#define EFIDP_HID_PCIROOT EISA_PNP_ID ( 0x0a03 )

/** Construct PciRoot(domain) device path */
#define EFIDP_PCIROOT( domain ) {					\
	.Header = EFIDP_HDR ( ACPI_DEVICE_PATH, ACPI_DP,		\
			      sizeof ( ACPI_HID_DEVICE_PATH ) ),	\
	.HID = EFIDP_HID_PCIROOT,					\
	.UID = (domain),						\
	}

/** Construct Pci(dev,fn) device path */
#define EFIDP_PCI( device, function ) {					\
	.Header = EFIDP_HDR ( HARDWARE_DEVICE_PATH, HW_PCI_DP,		\
			      sizeof ( PCI_DEVICE_PATH ) ),		\
	.Function = (function),						\
	.Device = (device),						\
	}

/** Construct Ata(secondary, slave, lun) device path */
#define EFIDP_ATA( secondary, slave, lun ) {				\
	.Header = EFIDP_HDR ( MESSAGING_DEVICE_PATH, MSG_ATAPI_DP,	\
			      sizeof ( ATAPI_DEVICE_PATH ) ),		\
	.PrimarySecondary = (secondary),				\
	.SlaveMaster = (slave),						\
	.Lun = (lun),							\
	}

/** Wrap MAC address bytes for inclusion in EFIDP_MAC() */
#define EFIDP_MAC_ADDR(...) { { __VA_ARGS__ } }

/** Construct MAC(address,type) device path */
#define EFIDP_MAC( address, type ) {					\
	.Header = EFIDP_HDR ( MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP,	\
			      sizeof ( MAC_ADDR_DEVICE_PATH ) ),	\
	.MacAddress = EFIDP_MAC_ADDR address,				\
	.IfType = (type),						\
	}

/** Construct IPv4(0.0.0.0) autoconfiguration device path */
#define EFIDP_IPv4_AUTO {						\
	.Header = EFIDP_HDR ( MESSAGING_DEVICE_PATH, MSG_IPv4_DP,	\
			      sizeof ( IPv4_DEVICE_PATH ) ),		\
	}

/** Construct Uri(...) device path header */
#define EFIDP_URI_HDR( len ) {						\
	.Header = EFIDP_HDR ( MESSAGING_DEVICE_PATH, MSG_URI_DP,	\
			      sizeof ( URI_DEVICE_PATH ) + (len) ),	\
	}

/** Wrap HD signature bytes for inclusion in EFIDP_HD_GPT() */
#define EFIDP_HD_SIG(...) { __VA_ARGS__ }

/** Construct HD(partition, GPT, start, size, signature) device path */
#define EFIDP_HD_GPT( partition, start, size, signature ) {		\
	.Header = EFIDP_HDR ( MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP,	\
			      sizeof ( HARDDRIVE_DEVICE_PATH ) ),	\
	.PartitionNumber = (partition),					\
	.PartitionStart = (start),					\
	.PartitionSize = (size),					\
	.Signature = EFIDP_HD_SIG signature,				\
	.MBRType = MBR_TYPE_EFI_PARTITION_TABLE_HEADER,			\
	.SignatureType = SIGNATURE_TYPE_GUID,				\
	}

/** Construct file device path header */
#define EFIDP_FILE_HDR( len )						\
	EFIDP_HDR ( MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP,		\
		    ( SIZE_OF_FILEPATH_DEVICE_PATH + (len) ) )

/** Construct FvFile(guid) device path */
#define EFIDP_FVFILE( guid ) {						\
	.Header = EFIDP_HDR ( MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_FILE_DP,	\
			      sizeof ( MEDIA_FW_VOL_FILEPATH_DEVICE_PATH ) ), \
	.FvFileName = guid,						\
	}

/** Construct Fv(guid) device path */
#define EFIDP_FV( guid ) {						\
	.Header = EFIDP_HDR ( MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_VOL_DP,	\
			      sizeof ( MEDIA_FW_VOL_DEVICE_PATH ) ),	\
	.FvName = guid,							\
	}

extern bool efidp_valid ( const void *path, size_t max_len );
extern size_t efidp_len ( const EFI_DEVICE_PATH_PROTOCOL *path );
extern EFI_DEVICE_PATH_PROTOCOL * efidp_from_text ( const char *text );
extern char * efidp_to_text ( const EFI_DEVICE_PATH_PROTOCOL *path,
			      bool display_only, bool allow_shortcuts );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _EFIDEVPATH_H */
