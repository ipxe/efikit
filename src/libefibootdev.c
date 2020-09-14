/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * EFI boot device selection library
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
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
	/** Modification flag */
	bool modified;
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
	/** Variable name */
	char name[EFIBOOT_NAME_LEN];
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
	if ( remaining < sizeof ( *option ) ) {
		errno = EINVAL;
		goto err_sanity;
	}
	desc = ( ( ( void * ) option ) + sizeof ( *option ) );
	remaining -= sizeof ( *option );
	desclen = StrnSizeS ( desc, remaining );
	if ( desclen > remaining ) {
		errno = EINVAL;
		goto err_sanity;
	}
	path = ( ( ( void * ) desc ) + desclen );
	remaining -= desclen;
	if ( option->FilePathListLength > remaining ) {
		errno = EINVAL;
		goto err_sanity;
	}

	/* Validate device path list and count device paths */
	remaining = option->FilePathListLength;
	count = 0;
	while ( remaining ) {
		if ( ! efidp_valid ( path, remaining ) ) {
			errno = EINVAL;
			goto err_sanity;
		}
		remaining -= efidp_len ( path );
		path = ( ( ( void * ) path ) + efidp_len ( path ) );
		count++;
	}
	if ( ! count ) {
		errno = EINVAL;
		goto err_sanity;
	}

	/* Allocate and initialise entry */
	entry = malloc ( sizeof ( *entry ) );
	if ( ! entry )
		goto err_entry;
	memset ( entry, 0, sizeof ( *entry ) );
	entry->modified = false;
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
 * Construct EFI variable name
 *
 * @v type		Load option type
 * @v index		Load option index
 * @v buf		Variable name buffer
 * @ret ok		Success indicator
 */
static int efiboot_index_name ( enum efi_boot_option_type type,
				unsigned int index, char *buf ) {

	/* Sanity checks */
	if ( ( type < 0 ) || ( type > EFIBOOT_TYPE_MAX ) ) {
		errno = EINVAL;
		return 0;
	}
	if ( index > EFIBOOT_INDEX_MAX ) {
		errno = EINVAL;
		return 0;
	}

	/* Construct name */
	snprintf ( buf, EFIBOOT_NAME_LEN, "%s%04X",
		   efiboot_prefix[type], index );

	return 1;
}

/**
 * Construct EFI order variable name
 *
 * @v type		Load option type
 * @v buf		Variable name buffer
 * @ret ok		Success indicator
 */
static int efiboot_order_name ( enum efi_boot_option_type type, char *buf ) {

	/* Sanity checks */
	if ( ( type < 0 ) || ( type > EFIBOOT_TYPE_MAX ) ) {
		errno = EINVAL;
		return 0;
	}

	/* Construct name */
	snprintf ( buf, EFIBOOT_NAME_LEN, "%sOrder", efiboot_prefix[type] );

	return 1;
}

/**
 * Set load option type and index
 *
 * @v entry		EFI boot entry
 * @v type		Load option type
 * @v index		Index
 * @ret ok		Success indicator
 */
static int efiboot_set_type_index ( struct efi_boot_entry *entry,
				    enum efi_boot_option_type type,
				    unsigned int index ) {

	/* Sanity checks */
	if ( ( type < 0 ) || ( type > EFIBOOT_TYPE_MAX ) ) {
		errno = EINVAL;
		return 0;
	}
	if ( ( index > EFIBOOT_INDEX_MAX ) &&
	     ( index != EFIBOOT_INDEX_AUTO ) ) {
		errno = EINVAL;
		return 0;
	}

	/* Set type */
	entry->type = type;

	/* Set index */
	entry->index = index;

	/* Update variable name */
	if ( ! efiboot_index_name ( type, index, entry->name ) )
		entry->name[0] = '\0';

	/* Mark as modified */
	entry->modified = true;

	return 1;
}

/**
 * Get variable name
 *
 * @v entry		EFI boot entry
 * @ret name		Variable name (or NULL on error)
 *
 * Note that a boot entry for which an index has not yet been assigned
 * will not have a variable name.
 */
const char * efiboot_name ( const struct efi_boot_entry *entry ) {
	return ( entry->name[0] ? entry->name : NULL );
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
	return efiboot_set_type_index ( entry, type, entry->index );
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
	return efiboot_set_type_index ( entry, entry->type, index );
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

	/* Set attributes */
	entry->attributes = attributes;

	/* Mark as modified */
	entry->modified = true;

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

	/* Mark as modified */
	entry->modified = true;

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
	if ( index >= entry->count ) {
		errno = EINVAL;
		return NULL;
	}

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
	if ( index >= entry->count ) {
		errno = EINVAL;
		return NULL;
	}

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
	if ( count < 1 ) {
		errno = EINVAL;
		return 0;
	}

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

	/* Mark as modified */
	entry->modified = true;

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
	if ( index >= entry->count ) {
		errno = EINVAL;
		goto err_count;
	}

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

	/* Mark as modified */
	entry->modified = true;

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
 * Create new EFI boot entry
 *
 * @ret entry		EFI boot entry, or NULL on error
 */
struct efi_boot_entry * efiboot_new ( void ) {
	struct efi_boot_entry *entry;
	static EFI_DEVICE_PATH_PROTOCOL path = EFIDP_END;
	static EFI_DEVICE_PATH_PROTOCOL *paths[] = { &path };

	/* Allocate entry */
	entry = malloc ( sizeof ( *entry ) );
	if ( ! entry )
		goto err_alloc;
	memset ( entry, 0, sizeof ( *entry ) );
	entry->modified = true;
	entry->type = EFIBOOT_TYPE_BOOT;
	entry->index = EFIBOOT_INDEX_AUTO;
	entry->attributes = LOAD_OPTION_ACTIVE;

	/* Set description */
	if ( ! efiboot_set_description ( entry, "Unknown" ) )
		goto err_description;

	/* Set device paths */
	if ( ! efiboot_set_paths ( entry, paths, 1 ) )
		goto err_paths;

	return entry;

 err_paths:
 err_description:
	efiboot_free ( entry );
 err_alloc:
	return NULL;
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

		/* Skip if already in use */
		if ( ! efiboot_index_name ( entry->type, index, name ) )
			continue;
		if ( efivars_exists ( name ) )
			continue;

		/* Record selected index */
		entry->index = index;

		/* Mark as modified */
		entry->modified = true;

		return 1;
	}

	errno = ENOSPC;
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
	if ( ! efiboot_index_name ( type, index, name ) )
		goto err_name;

	/* Read variable data */
	if ( ! efivars_read ( name, &data, &len ) )
		goto err_read;

	/* Parse boot entry */
	entry = efiboot_from_option ( data, len );
	if ( ! entry )
		goto err_from_option;

	/* Record type, index, and variable name */
	entry->type = type;
	entry->index = index;
	memcpy ( entry->name, name, sizeof ( entry->name ) );

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
	EFI_LOAD_OPTION *option;
	size_t len;

	/* Skip saving if entry is unmodified */
	if ( ! entry->modified )
		return 1;

	/* Select index, if applicable */
	if ( entry->index == EFIBOOT_INDEX_AUTO ) {
		if ( ! efiboot_autoindex ( entry ) )
			goto err_autoindex;
	}

	/* Construct load option */
	option = efiboot_to_option ( entry, &len );
	if ( ! option )
		goto err_to_option;

	/* Write variable data */
	if ( ! efivars_write ( efiboot_name ( entry ), option, len ) )
		goto err_write;

	/* Free load option */
	free ( option );

	/* Clear modification flag */
	entry->modified = false;

	return 1;

 err_write:
	free ( option );
 err_to_option:
 err_autoindex:
	return 0;
}

/**
 * Free EFI boot entry list
 *
 * @v entries		List of boot entries
 */
void efiboot_free_all ( struct efi_boot_entry **entries ) {
	struct efi_boot_entry **tmp;

	for ( tmp = entries ; *tmp ; tmp++ )
		efiboot_free ( *tmp );
	free ( entries );
}

/**
 * Load EFI boot entry list from EFI variables
 *
 * @v type		Load option type
 * @ret entries		List of boot entries (NULL terminated), or NULL on error
 *
 * The list of boot entries is dynamically allocated and must
 * eventually be freed by the caller using efiboot_free_all().
 */
struct efi_boot_entry ** efiboot_load_all ( enum efi_boot_option_type type ) {
	struct efi_boot_entry **entries;
	char name[EFIBOOT_NAME_LEN];
	void *data;
	size_t len;
	uint16_t *index;
	int count;
	int i;

	/* Construct order variable name */
	if ( ! efiboot_order_name ( type, name ) )
		goto err_name;

	/* Read order variable
	 *
	 * Zero-length variables are not supported.  Treat a missing
	 * order variable as equivalent to an empty list.
	 */
	if ( ! efivars_read ( name, &data, &len ) ) {
		if ( errno != ENOENT )
			goto err_read;
		data = NULL;
		len = 0;
	}

	/* Allocate list of entries */
	index = data;
	count = ( len / sizeof ( index[0] ) );
	entries = malloc ( ( count + 1 /* NULL */ ) * sizeof ( entries[0] ) );
	if ( ! entries )
		goto err_alloc;
	for ( i = 0 ; i < count ; i++ ) {
		entries[i] = efiboot_load ( type, index[i] );
		if ( ! entries[i] )
			goto err_load;
	}
	entries[count] = NULL;

	/* Free order variable */
	free ( data );

	return entries;

 err_load:
	for ( i-- ; i >= 0 ; i-- )
		efiboot_free ( entries[i] );
	free ( entries );
 err_alloc:
	free ( data );
 err_read:
 err_name:
	return NULL;
}

/**
 * Save EFI boot entry list to EFI variables
 *
 * @v type		Load option type
 * @v entries		List of boot entries (NULL terminated)
 * @ret ok		Success indicator
 *
 * If any boot entry index is @c EFIBOOT_INDEX_AUTO then it will be
 * updated to reflect the automatically selected index.
 */
int efiboot_save_all ( enum efi_boot_option_type type,
		       struct efi_boot_entry **entries ) {
	char name[EFIBOOT_NAME_LEN];
	uint16_t *index;
	size_t len;
	unsigned int count;
	unsigned int i;

	/* Construct order variable name */
	if ( ! efiboot_order_name ( type, name ) )
		goto err_name;

	/* Count number of entries */
	for ( count = 0 ; entries[count] ; count++ ) {}

	/* Save each individual entry */
	for ( i = 0 ; i < count ; i++ ) {
		if ( entries[i]->type != type ) {
			errno = EINVAL;
			goto err_type;
		}
		if ( ! efiboot_save ( entries[i] ) )
			goto err_save;
	}

	/* Allocate order variable */
	len = ( count * sizeof ( index[0] ) );
	index = malloc ( len );
	if ( ! index )
		goto err_alloc;

	/* Construct order variable */
	for ( i = 0 ; i < count ; i++ )
		index[i] = entries[i]->index;

	/* Save order variable */
	if ( ! efivars_write ( name, index, len ) )
		goto err_write;

	/* Free order variable */
	free ( index );

	return 1;

 err_write:
	free ( index );
 err_alloc:
 err_save:
 err_type:
 err_name:
	return 0;
}
