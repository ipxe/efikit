/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Conversion between UTF-8 and EFI UCS2-LE strings
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <iconv.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>

#include "strconvert.h"
#include "config.h"

/** Buffer size increment */
#define BUFSZ 512

/**
 * Convert string between character encodings using iconv
 *
 * @v in		Input string
 * @v inlen		Length of input string (in bytes)
 * @v incode		Input string encoding
 * @v outcode		Output string encoding
 * @ret out		Output string, or NULL on error
 *
 * The output string is allocated using malloc() and must eventually
 * be freed by the caller.
 */
static void * convert_string ( ICONV_CONST char *in, size_t inlen,
			       const char *incode, const char *outcode ) {
	void *buf = NULL;
	size_t len = 0;
	size_t outlen = 0;
	char *out;
	void *tmp;
	iconv_t cd;

	/* Open conversion */
	if ( ( cd = iconv_open ( outcode, incode ) ) == ( ( iconv_t ) -1 ) )
		goto err_open;

	/* Convert string */
	while ( 1 ) {

		/* (Re)allocate output buffer */
		len += BUFSZ;
		tmp = realloc ( buf, len );
		if ( ! tmp )
			goto err_realloc;
		buf = tmp;

		/* Update output pointer */
		outlen += BUFSZ;
		out = ( buf + len - outlen );

		/* Convert as much input as possible */
		if ( iconv ( cd, &in, &inlen, &out,
			     &outlen ) != ( ( size_t ) -1 ) ) {
			break;
		}
		if ( errno != E2BIG )
			goto err_convert;
	}

	/* Close conversion */
	iconv_close ( cd );

	return buf;

 err_convert:
 err_realloc:
	free ( buf );
	iconv_close ( cd );
 err_open:
	return NULL;
}

/**
 * Convert UTF-8 string to EFI UCS2-LE string
 *
 * @v in		Input string
 * @ret out		Output string, or NULL on error
 *
 * The output string is allocated using malloc() and must eventually
 * be freed by the caller.
 */
CHAR16 * utf8_to_efi ( char *utf8 ) {
	size_t len = ( strlen ( utf8 ) + 1 /* NUL */ );
	return convert_string ( utf8, len, "UTF8", "UCS-2LE" );
}

/**
 * Convert EFI UCS2-LE string to UTF-8 string
 *
 * @v in		Input string
 * @ret out		Output string, or NULL on error
 *
 * The output string is allocated using malloc() and must eventually
 * be freed by the caller.
 */
char * efi_to_utf8 ( CHAR16 *efi ) {
	size_t len = ( ( StrLen ( efi ) + 1 /* wNUL */ ) * sizeof ( efi[0] ) );
	return convert_string ( ( ( char * ) efi ), len, "UCS-2LE", "UTF8" );
}
