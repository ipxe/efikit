/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * MemoryAllocationLib compatible API self-tests
 *
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>
#include <Uefi/UefiBaseType.h>
#include <Library/MemoryAllocationLib.h>

#include "memalloctest.h"

/**
 * Fill memory with repeatably pseudorandom bytes
 *
 * @v data		Data buffer
 * @v len		Length of data buffer
 */
static void memset_random ( void *data, size_t len ) {
	static unsigned int seed = 42;
	uint8_t *bytes = data;
	unsigned int tmp;
	unsigned int i;

	/* Fill with repeatably pseudorandom values
	 *
	 * This would be cleaner to implement with rand_r(), but that
	 * is not available on Windows.
	 */
	tmp = rand();
	srand ( seed );
	for ( i = 0 ; i < len ; i++ )
		bytes[i] = rand();
	seed = rand();
	srand ( tmp );
}

/**
 * Test allocation result
 *
 * @v data		Allocated data buffer
 * @v alignment		Expected alignment
 */
static void assert_allocated ( void *data, size_t alignment ) {
	size_t offset;

	/* Check allocation was successful */
	assert_non_null ( data );

	/* Check alignment was correct */
	offset = ( ( ( intptr_t ) data ) & ( alignment - 1 ) );
	assert_int_equal ( offset, 0 );
}

/**
 * Test allocation via AllocatePages or similar
 *
 * @v allocfn		Allocation function
 * @v pages		Number of pages to allocate
 */
static void assert_alloc_pages ( typeof ( AllocatePages ) *allocfn,
				 unsigned int pages ) {
	void *data;

	/* Check allocation */
	data = allocfn ( pages );
	assert_allocated ( data, EFI_PAGE_SIZE );

	/* Fill memory to trigger valgrind errors on under-allocation */
	memset_random ( data, ( pages * EFI_PAGE_SIZE ) );

	/* Free data */
	FreePages ( data, pages );
}

/**
 * Test allocation via AllocateAlignedPages or similar
 *
 * @v allocfn		Allocation function
 * @v pages		Number of pages to allocate
 * @v alignment		Required alignment
 */
static void
assert_alloc_aligned_pages ( typeof ( AllocateAlignedPages ) *allocfn,
			     unsigned int pages, unsigned int alignment ) {
	void *data;

	/* Check allocation */
	data = allocfn ( pages, alignment );
	assert_allocated ( data, EFI_PAGE_SIZE );

	/* Fill memory to trigger valgrind errors on under-allocation */
	memset_random ( data, ( pages * EFI_PAGE_SIZE ) );

	/* Free data */
	FreeAlignedPages ( data, pages );
}

/**
 * Test allocation via AllocatePool or similar
 *
 * @v allocfn		Allocation function
 * @v zerofn		Zeroed allocation function
 * @v copyfn		Copy allocation function
 * @v reallocfn		Reallocation function
 * @v len		Length to allocate
 */
static void assert_alloc_pool ( typeof ( AllocatePool ) *allocfn,
				typeof ( AllocateZeroPool ) *zerofn,
				typeof ( AllocateCopyPool ) *copyfn,
				typeof ( ReallocatePool ) *reallocfn,
				size_t len ) {
	void *data;
	void *zdata;
	void *cdata;
	void *rdata;
	uint8_t *bytes;
	unsigned int i;

	/* Test basic allocation */
	data = allocfn ( len );
	assert_allocated ( data, sizeof ( unsigned long ) );

	/* Test zeroed allocation */
	zdata = zerofn ( len );
	assert_allocated ( zdata, sizeof ( unsigned long ) );
	bytes = zdata;
	for ( i = 0 ; i < len ; i++ )
		assert_int_equal ( bytes[i], 0 );

	/* Test copy allocation */
	memset_random ( data, len );
	cdata = copyfn ( len, data );
	assert_allocated ( cdata, sizeof ( unsigned long ) );
	assert_memory_equal ( cdata, data, len );

	/* Test reallocation larger */
	rdata = reallocfn ( len, ( len * 3 ), data );
	assert_allocated ( rdata, sizeof ( unsigned long ) );
	assert_memory_equal ( rdata, cdata, len );

	/* Test reallocation smaller */
	rdata = reallocfn ( ( len * 3 ), ( len / 2 ), rdata );
	assert_allocated ( rdata, sizeof ( unsigned long ) );
	assert_memory_equal ( rdata, cdata, ( len / 2 ) );

	/* Free data */
	FreePool ( rdata );
	FreePool ( cdata );
	FreePool ( zdata );
}

/* Test memory allocations */
void test_memalloc ( void **state ) {

	/* Test page allocation */
	assert_alloc_pages ( AllocatePages, 3 );
	assert_alloc_pages ( AllocateRuntimePages, 4 );
	assert_alloc_pages ( AllocateReservedPages, 19 );

	/* Test aligned page allocation */
	assert_alloc_aligned_pages ( AllocateAlignedPages, 2, 0x10000 );
	assert_alloc_aligned_pages ( AllocateAlignedRuntimePages, 1, 0x4000 );
	assert_alloc_aligned_pages ( AllocateAlignedReservedPages, 24, 0x8000 );

	/* Test pool allocation */
	assert_alloc_pool ( AllocatePool, AllocateZeroPool,
			    AllocateCopyPool, ReallocatePool, 43 );
	assert_alloc_pool ( AllocateRuntimePool, AllocateRuntimeZeroPool,
			    AllocateRuntimeCopyPool, ReallocateRuntimePool,
			    81762 );
	assert_alloc_pool ( AllocateReservedPool, AllocateReservedZeroPool,
			    AllocateReservedCopyPool, ReallocateReservedPool,
			    1765 );

	( void ) state;
}
