/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot device selection library
 *
 */

#ifndef _EFIBOOTDEV_H
#define _EFIBOOTDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Protocol/DevicePath.h>

/** An EFI boot entry */
struct efi_boot_entry;

/** EFI boot load option types */
enum efi_boot_option_type {
	EFIBOOT_TYPE_BOOT = 0,
	EFIBOOT_TYPE_DRIVER,
	EFIBOOT_TYPE_SYSPREP,
	EFIBOOT_TYPE_MAX = EFIBOOT_TYPE_SYSPREP
};

/** Maximum valid boot index */
#define EFIBOOT_INDEX_MAX 0xffffU

/** Auto-assigned boot index */
#define EFIBOOT_INDEX_AUTO -1U

extern void efiboot_free ( struct efi_boot_entry *entry );
extern struct efi_boot_entry *
efiboot_from_option ( const EFI_LOAD_OPTION *option, size_t len );
extern EFI_LOAD_OPTION * efiboot_to_option ( const struct efi_boot_entry *entry,
					     size_t *len );
extern enum efi_boot_option_type
efiboot_type ( const struct efi_boot_entry *entry );
extern int efiboot_set_type ( struct efi_boot_entry *entry,
			      enum efi_boot_option_type type );
extern unsigned int efiboot_index ( const struct efi_boot_entry *entry );
extern int efiboot_set_index ( struct efi_boot_entry *entry,
			       unsigned int index );
extern uint32_t efiboot_attributes ( const struct efi_boot_entry *entry );
extern int efiboot_set_attributes ( struct efi_boot_entry *entry,
				    uint32_t attributes );
extern const char * efiboot_description ( const struct efi_boot_entry *entry );
extern int efiboot_set_description ( struct efi_boot_entry *entry,
				     const char *desc );
extern unsigned int efiboot_path_count ( const struct efi_boot_entry *entry );
extern const EFI_DEVICE_PATH_PROTOCOL *
efiboot_path ( const struct efi_boot_entry *entry, unsigned int index );
extern const char * efiboot_path_text ( const struct efi_boot_entry *entry,
					unsigned int index );
extern int efiboot_set_paths ( struct efi_boot_entry *entry,
			       EFI_DEVICE_PATH_PROTOCOL **paths,
			       unsigned int count );
extern int efiboot_set_path ( struct efi_boot_entry *entry, unsigned int index,
			      const EFI_DEVICE_PATH_PROTOCOL *path );
extern const void * efiboot_data ( const struct efi_boot_entry *entry );
extern size_t efiboot_data_len ( const struct efi_boot_entry *entry );
extern int efiboot_set_data ( struct efi_boot_entry *entry, const void *data,
			      size_t len );
extern void efiboot_clear_data ( struct efi_boot_entry *entry );
extern struct efi_boot_entry *
efiboot_new ( enum efi_boot_option_type type, unsigned int index,
	      uint32_t attributes, const char *description,
	      EFI_DEVICE_PATH_PROTOCOL **paths, unsigned int count,
	      const void *data, size_t len );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _EFIBOOTDEV_H */
