/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot device selection library
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Library/BaseLib.h>
#include <efidevpath.h>
#include <efibootdev.h>

#include "strconvert.h"
#include "efivars.h"

/** Boot variable name prefixes */
static const char *efiboot_prefix[] = {
	[EFIBOOT_TYPE_BOOT] = "Boot",
	[EFIBOOT_TYPE_DRIVER] = "Driver",
	[EFIBOOT_TYPE_SYSPREP] = "SysPrep",
};

/** Maximum length of a boot variable name */
#define EFIBOOT_NAME_LEN \
	( 7 /* "SysPrep" */ + 5 /* "Order" */ + 1 /* NUL */ )

/** An EFI boot entry device path */
struct efi_boot_entry_path {
	/** Device path protocol */
	EFI_DEVICE_PATH_PROTOCOL *path;
	/** Cached canonical textual representation (as UTF8 string) */
	char *text;
};

/** An EFI boot entry */
struct efi_boot_entry {
	/** Type */
	enum efi_boot_option_type type;
	/** Index */
	unsigned int index;
	/** Attributes */
	uint32_t attributes;
	/** Description (as UTF8 string) */
	char *description;
	/** Device paths */
	struct efi_boot_entry_path *paths;
	/** Number of device paths */
	unsigned int count;
	/** Optional data */
	void *data;
	/** Length of optional data */
	size_t len;
};

/**
 * Free EFI boot entry cached device path textual representations
 *
 * @v entry		EFI boot entry
 */
static void efiboot_free_text ( struct efi_boot_entry *entry ) {
	unsigned int i;

	for ( i = 0 ; i < entry->count ; i++ ) {
		free ( entry->paths[i].text );
		entry->paths[i].text = NULL;
	}
}

/**
 * Free EFI boot entry
 *
 * @v entry		EFI boot entry
 */
void efiboot_free ( struct efi_boot_entry *entry ) {

	efiboot_free_text ( entry );
	free ( entry->data );
	free ( entry->paths );
	free ( entry->description );
	free ( entry );
}

/**
 * Parse EFI load option
 *
 * @v option		EFI load option
 * @v len		Length of EFI load option
 * @ret entry		EFI boot entry (or NULL on error)
 *
 * The boot entry is an opaque structure including embedded pointers
 * to dynamically allocated memory.  It must eventually be freed by
 * the caller using efiboot_free().
 */
struct efi_boot_entry * efiboot_from_option ( const EFI_LOAD_OPTION *option,
					      size_t len ) {
	struct efi_boot_entry *entry;
	EFI_DEVICE_PATH_PROTOCOL *path;
	CHAR16 *desc;
	size_t desclen;
	size_t remaining;
	unsigned int count;
	unsigned int i;

	/* Validate load option */
	remaining = len;
	if ( remaining < sizeof ( *option ) )
		goto err_sanity;
	desc = ( ( ( void * ) option ) + sizeof ( *option ) );
	remaining -= sizeof ( *option );
	desclen = StrnSizeS ( desc, remaining );
	if ( desclen > remaining )
		goto err_sanity;
	path = ( ( ( void * ) desc ) + desclen );
	remaining -= desclen;
	if ( option->FilePathListLength > remaining )
		goto err_sanity;

	/* Validate device path list and count device paths */
	remaining = option->FilePathListLength;
	count = 0;
	while ( remaining ) {
		if ( ! efidp_valid ( path, remaining ) )
			goto err_sanity;
		remaining -= efidp_len ( path );
		path = ( ( ( void * ) path ) + efidp_len ( path ) );
		count++;
	}
	if ( ! count )
		goto err_sanity;

	/* Allocate and initialise entry */
	entry = malloc ( sizeof ( *entry ) );
	if ( ! entry )
		goto err_entry;
	memset ( entry, 0, sizeof ( *entry ) );
	entry->type = EFIBOOT_TYPE_BOOT;
	entry->index = EFIBOOT_INDEX_AUTO;
	entry->attributes = option->Attributes;

	/* Populate description */
	desc = ( ( ( void * ) option ) + sizeof ( *option ) );
	entry->description = efi_to_utf8 ( desc );
	if ( ! entry->description )
		goto err_description;

	/* Populate device paths */
	entry->paths = malloc ( ( count * sizeof ( entry->paths[0] ) ) +
				option->FilePathListLength );
	if ( ! entry->paths )
		goto err_paths;
	path = ( ( ( void * ) entry->paths ) +
		 ( count * sizeof ( entry->paths[0] ) ) );
	memcpy ( path, ( ( ( void * ) desc ) + desclen ),
		 option->FilePathListLength );
	for ( i = 0 ; i < count ; i++ ) {
		entry->paths[i].path = path;
		entry->paths[i].text = NULL;
		path = ( ( ( void * ) path ) + efidp_len ( path ) );
	}
	entry->count = count;

	/* Populate optional data */
	entry->len = ( len - sizeof ( *option ) - desclen -
		       option->FilePathListLength );
	if ( entry->len ) {
		entry->data = malloc ( entry->len );
		if ( ! entry->data )
			goto err_data;
		memcpy ( entry->data, ( ( void * ) option + len - entry->len ),
			 entry->len );
	}

	return entry;

	free ( entry->data );
 err_data:
	free ( entry->paths );
 err_paths:
	free ( entry->description );
 err_description:
	free ( entry );
 err_entry:
 err_sanity:
	return NULL;
}

/**
 * Construct EFI load option
 *
 * @v entry		EFI boot entry
 * @v len		Length of EFI load option to fill in
 * @ret option		EFI load option (or NULL on error)
 *
 * The load option is allocated using malloc() and must eventually be
 * freed by the caller.
 */
EFI_LOAD_OPTION * efiboot_to_option ( const struct efi_boot_entry *entry,
				      size_t *len ) {
	EFI_LOAD_OPTION *option;
	CHAR16 *desc;
	size_t desclen;
	size_t pathlen;
	size_t pathslen;
	unsigned int i;
	void *tmp;

	/* Convert description to EFI string */
	desc = utf8_to_efi ( entry->description );
	if ( ! desc )
		goto err_desc;

	/* Calculate required lengths */
	desclen = StrSize ( desc );
	pathslen = 0;
	for ( i = 0 ; i < entry->count ; i++ )
		pathslen += efidp_len ( entry->paths[i].path );
	*len = ( sizeof ( *option ) + desclen + pathslen + entry->len );

	/* Allocate option */
	option = malloc ( *len );
	if ( ! option )
		goto err_alloc;
	tmp = ( ( ( void * ) option ) + sizeof ( *option ) );

	/* Populate option */
	option->Attributes = entry->attributes;
	option->FilePathListLength = pathslen;
	memcpy ( tmp, desc, desclen );
	tmp += desclen;
	for ( i = 0 ; i < entry->count ; i++ ) {
		pathlen = efidp_len ( entry->paths[i].path );
		memcpy ( tmp, entry->paths[i].path, pathlen );
		tmp += pathlen;
	}
	memcpy ( tmp, entry->data, entry->len );

	/* Free EFI string */
	free ( desc );

	return option;

	free ( option );
 err_alloc:
	free ( desc );
 err_desc:
	return NULL;
}

/**
 * Get load option type
 *
 * @v entry		EFI boot entry
 * @ret type		Load option type
 */
enum efi_boot_option_type efiboot_type ( const struct efi_boot_entry *entry ) {
	return entry->type;
}

/**
 * Set load option type
 *
 * @v entry		EFI boot entry
 * @v type		Load option type
 * @ret ok		Success indicator
 */
int efiboot_set_type ( struct efi_boot_entry *entry,
		       enum efi_boot_option_type type ) {

	/* Sanity check */
	if ( ( type < 0 ) || ( type > EFIBOOT_TYPE_MAX ) )
		return 0;

	/* Set type */
	entry->type = type;

	return 1;
}

/**
 * Get index
 *
 * @v entry		EFI boot entry
 * @ret index		Index (or @c EFIBOOT_INDEX_AUTO)
 */
unsigned int efiboot_index ( const struct efi_boot_entry *entry ) {
	return entry->index;
}

/**
 * Set index
 *
 * @v entry		EFI boot entry
 * @v index		Index (or @c EFIBOOT_INDEX_AUTO)
 */
int efiboot_set_index ( struct efi_boot_entry *entry, unsigned int index ) {

	/* Sanity check */
	if ( ( index > EFIBOOT_INDEX_MAX ) && ( index != EFIBOOT_INDEX_AUTO ) )
		return 0;

	/* Set index */
	entry->index = index;

	return 1;
}

/**
 * Get attributes
 *
 * @v entry		EFI boot entry
 * @ret attributes	Attributes
 */
uint32_t efiboot_attributes ( const struct efi_boot_entry *entry ) {
	return entry->attributes;
}

/**
 * Set attributes
 *
 * @v entry		EFI boot entry
 * @v attributes	Attributes
 * @ret ok		Success indicator
 */
int efiboot_set_attributes ( struct efi_boot_entry *entry,
			     uint32_t attributes ) {
	entry->attributes = attributes;
	return 1;
}

/**
 * Get description
 *
 * @v entry		EFI boot entry
 * @ret desc		Description (as UTF8 string)
 */
const char * efiboot_description ( const struct efi_boot_entry *entry ) {
	return entry->description;
}

/**
 * Set description
 *
 * @v entry		EFI boot entry
 * @v desc		Description (as UTF8 string)
 * @ret ok		Success indicator
 */
int efiboot_set_description ( struct efi_boot_entry *entry,
			      const char *desc ) {
	char *tmp;

	/* Copy description */
	tmp = strdup ( desc );
	if ( ! tmp )
		return 0;

	/* Free old description */
	free ( entry->description );

	/* Update description */
	entry->description = tmp;

	return 1;
}

/**
 * Get number of device paths
 *
 * @v entry		EFI boot entry
 * @ret count		Number of device paths (will always be at least 1)
 */
unsigned int efiboot_path_count ( const struct efi_boot_entry *entry ) {
	return entry->count;
}

/**
 * Get device path
 *
 * @v entry		EFI boot entry
 * @v index		Path index
 * @ret path		Device path (or NULL on index overflow)
 *
 * Path index 0 is guaranteed to always exist.
 */
const EFI_DEVICE_PATH_PROTOCOL *
efiboot_path ( const struct efi_boot_entry *entry, unsigned int index ) {

	/* Sanity check */
	if ( index >= entry->count )
		return NULL;

	return entry->paths[index].path;
}

/**
 * Get device path canonical textual representation
 *
 * @v entry		EFI boot entry
 * @v index		Path index
 * @ret path		Device path (or NULL on error)
 *
 * Path index 0 is guaranteed to always exist.
 */
const char * efiboot_path_text ( const struct efi_boot_entry *entry,
				 unsigned int index ) {
	struct efi_boot_entry_path *path;

	/* Sanity check */
	if ( index >= entry->count )
		return NULL;

	/* Create cached representation if needed */
	path = &entry->paths[index];
	if ( ! path->text )
		path->text = efidp_to_text ( path->path, false, true );

	return path->text;
}

/**
 * Set device paths
 *
 * @v entry		EFI boot entry
 * @v paths		Device paths
 * @v count		Number of device paths (must be at least 1)
 * @ret ok		Success indicator
 */
int efiboot_set_paths ( struct efi_boot_entry *entry,
			EFI_DEVICE_PATH_PROTOCOL **paths, unsigned int count ) {
	struct efi_boot_entry_path *tmp;
	EFI_DEVICE_PATH_PROTOCOL *path;
	size_t len;
	unsigned int i;

	/* Sanity check */
	if ( count < 1 )
		return 0;

	/* Copy device paths */
	len = 0;
	for ( i = 0 ; i < count ; i++ )
		len += efidp_len ( paths[i] );
	tmp = malloc ( ( count * sizeof ( tmp[0] ) ) + len );
	if ( ! tmp )
		return 0;
	path = ( ( ( void * ) tmp ) + ( count * sizeof ( tmp[0] ) ) );
	for ( i = 0 ; i < count ; i++ ) {
		tmp[i].path = path;
		tmp[i].text = NULL;
		len = efidp_len ( paths[i] );
		memcpy ( path, paths[i], len );
		path = ( ( ( void * ) path ) + len );
	}

	/* Free old device paths */
	efiboot_free_text ( entry );
	free ( entry->paths );

	/* Update device paths */
	entry->paths = tmp;
	entry->count = count;

	return 1;
}

/**
 * Set device path
 *
 * @v entry		EFI boot entry
 * @v index		Path index
 * @v path		Device path
 * @ret ok		Success indicator
 */
int efiboot_set_path ( struct efi_boot_entry *entry, unsigned int index,
		       const EFI_DEVICE_PATH_PROTOCOL *path ) {
	EFI_DEVICE_PATH_PROTOCOL **paths;
	unsigned int i;

	/* Sanity check */
	if ( index >= entry->count )
		goto err_count;

	/* Construct updated list of device paths */
	paths = malloc ( entry->count * sizeof ( paths[0] ) );
	if ( ! paths )
		goto err_alloc;
	for ( i = 0 ; i < entry->count ; i++ )
		paths[i] = entry->paths[i].path;
	paths[index] = ( ( EFI_DEVICE_PATH_PROTOCOL * ) path );

	/* Set device paths */
	if ( ! efiboot_set_paths ( entry, paths, entry->count ) )
		goto err_set_paths;

	return 1;

 err_set_paths:
	free ( paths );
 err_alloc:
 err_count:
	return 0;
}

/**
 * Get optional data
 *
 * @v entry		EFI boot entry
 * @ret data		Optional data (may be NULL)
 */
const void * efiboot_data ( const struct efi_boot_entry *entry ) {
	return entry->data;
}

/**
 * Get length of optional data
 *
 * @v entry		EFI boot entry
 * @ret len		Length of optional data
 */
size_t efiboot_data_len ( const struct efi_boot_entry *entry ) {
	return entry->len;
}

/**
 * Set optional data
 *
 * @v entry		EFI boot entry
 * @v data		Optional data (NULL to clear optional data)
 * @v len		Length of optional data (0 to clear optional data)
 * @ret ok		Success indicator
 */
int efiboot_set_data ( struct efi_boot_entry *entry, const void *data,
		       size_t len ) {
	void *tmp;

	/* Copy data */
	if ( len ) {
		tmp = malloc ( len );
		if ( ! tmp )
			return 0;
		memcpy ( tmp, data, len );
	} else {
		tmp = NULL;
	}

	/* Free old optional data */
	free ( entry->data );

	/* Update optional data */
	entry->data = tmp;
	entry->len = len;

	return 1;
}

/**
 * Clear optional data
 *
 * @v entry		EFI boot entry
 */
void efiboot_clear_data ( struct efi_boot_entry *entry ) {
	efiboot_set_data ( entry, NULL, 0 );
}

/**
 * Create EFI boot entry
 *
 * @v type		Load option type
 * @v index		Index (or @c EFIBOOT_INDEX_AUTO)
 * @v attributes	Attributes
 * @v description	Description (as UTF8 string)
 * @v paths		Device paths
 * @v count		Number of device paths (must be at least 1)
 * @v data		Optional data (NULL if no optional data)
 * @v len		Length of optional data (0 if no optional data)
 * @ret entry		EFI boot entry, or NULL on error
 */
struct efi_boot_entry * efiboot_new ( enum efi_boot_option_type type,
				      unsigned int index, uint32_t attributes,
				      const char *description,
				      EFI_DEVICE_PATH_PROTOCOL **paths,
				      unsigned int count, const void *data,
				      size_t len ) {
	struct efi_boot_entry *entry;

	/* Allocate entry */
	entry = malloc ( sizeof ( *entry ) );
	if ( ! entry )
		goto err_alloc;
	memset ( entry, 0, sizeof ( *entry ) );

	/* Set type */
	if ( ! efiboot_set_type ( entry, type ) )
		goto err_type;

	/* Set index */
	if ( ! efiboot_set_index ( entry, index ) )
		goto err_index;

	/* Set attributes */
	if ( ! efiboot_set_attributes ( entry, attributes ) )
		goto err_attributes;

	/* Set description */
	if ( ! efiboot_set_description ( entry, description ) )
		goto err_description;

	/* Set device paths */
	if ( ! efiboot_set_paths ( entry, paths, count ) )
		goto err_paths;

	/* Set optional data */
	if ( ! efiboot_set_data ( entry, data, len ) )
		goto err_data;

	return entry;

 err_data:
 err_paths:
 err_description:
 err_attributes:
 err_index:
 err_type:
	efiboot_free ( entry );
 err_alloc:
	return NULL;
}

/**
 * Construct EFI variable name
 *
 * @v type		Load option type
 * @v index		Load option index
 * @v buf		Variable name buffer
 * @ret ok		Success indicator
 */
static int efiboot_name ( enum efi_boot_option_type type, unsigned int index,
			  char *buf ) {

	/* Sanity checks */
	if ( ( type < 0 ) || ( type > EFIBOOT_TYPE_MAX ) )
		return 0;
	if ( index > EFIBOOT_INDEX_MAX )
		return 0;

	/* Construct name */
	snprintf ( buf, EFIBOOT_NAME_LEN, "%s%04x",
		   efiboot_prefix[type], index );

	return 1;
}

/**
 * Automatically assign EFI variable index
 *
 * @v entry		EFI boot entry
 * @ret ok		Success indicator
 */
static int efiboot_autoindex ( struct efi_boot_entry *entry ) {
	char name[EFIBOOT_NAME_LEN];
	unsigned int index;

	/* Find an unused index */
	for ( index = 0 ; index <= EFIBOOT_INDEX_MAX ; index++ ) {
		if ( ! efiboot_name ( entry->type, index, name ) )
			continue;
		if ( efivars_exists ( name ) )
			continue;
		entry->index = index;
		return 1;
	}

	return 0;
}

/**
 * Load boot entry from EFI variable
 *
 * @v type		Load option type
 * @v index		Load option index
 * @ret entry		EFI boot entry, or NULL on error
 *
 * The boot entry is an opaque structure including embedded pointers
 * to dynamically allocated memory.  It must eventually be freed by
 * the caller using efiboot_free().
 */
struct efi_boot_entry * efiboot_load ( enum efi_boot_option_type type,
				       unsigned int index ) {
	struct efi_boot_entry *entry;
	char name[EFIBOOT_NAME_LEN];
	void *data;
	size_t len;

	/* Construct variable name */
	if ( ! efiboot_name ( type, index, name ) )
		goto err_name;

	/* Read variable data */
	if ( ! efivars_read ( name, &data, &len ) )
		goto err_read;

	/* Parse boot entry */
	entry = efiboot_from_option ( data, len );
	if ( ! entry )
		goto err_from_option;

	/* Record type and index */
	entry->type = type;
	entry->index = index;

	/* Free variable data */
	free ( data );

	return entry;

 err_from_option:
	free ( data );
 err_read:
 err_name:
	return NULL;
}

/**
 * Save boot entry to EFI variable
 *
 * @v entry		EFI boot entry
 * @ret ok		Success indicator
 *
 * If the boot entry index is @c EFIBOOT_INDEX_AUTO then it will be
 * updated to reflect the automatically selected index.
 */
int efiboot_save ( struct efi_boot_entry *entry ) {
	char name[EFIBOOT_NAME_LEN];
	EFI_LOAD_OPTION *option;
	size_t len;

	/* Select index, if applicable */
	if ( entry->index == EFIBOOT_INDEX_AUTO ) {
		if ( ! efiboot_autoindex ( entry ) )
			goto err_autoindex;
	}

	/* Construct variable name */
	if ( ! efiboot_name ( entry->type, entry->index, name ) )
		goto err_name;

	/* Construct load option */
	option = efiboot_to_option ( entry, &len );
	if ( ! option )
		goto err_to_option;

	/* Write variable data */
	if ( ! efivars_write ( name, option, len ) )
		goto err_write;

	/* Free load option */
	free ( option );

	return 1;

 err_write:
	free ( option );
 err_to_option:
 err_name:
 err_autoindex:
	return 0;
}
