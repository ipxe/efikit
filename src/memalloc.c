/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * MemoryAllocationLib compatible API providing memory allocation via
 * the standard platform malloc()/free() API.
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include "config.h"

/*****************************************************************************
 *
 * Platform memory allocation helper functions
 *
 *****************************************************************************
 */

#if HAVE_DECL_POSIX_MEMALIGN

STATIC VOID * PlatformAllocate ( IN UINTN AllocationSize, IN UINTN Alignment ) {
	VOID *Buffer;

	if ( Alignment < sizeof ( void * ) )
		Alignment = sizeof ( void * );
	if ( posix_memalign ( &Buffer, Alignment, AllocationSize ) != 0 )
		return NULL;
	return Buffer;
}

STATIC VOID PlatformFree ( IN VOID *Buffer ) {
	free ( Buffer );
}

#elif HAVE_DECL__ALIGNED_MALLOC

STATIC VOID * PlatformAllocate ( IN UINTN AllocationSize, IN UINTN Alignment ) {

	if ( Alignment < sizeof ( void * ) )
		Alignment = sizeof ( void * );
	return _aligned_malloc ( AllocationSize, Alignment );
}

STATIC VOID PlatformFree ( IN VOID *Buffer ) {
	_aligned_free ( Buffer );
}

#else

#error "No platform memory allocation found found"

#endif


/*****************************************************************************
 *
 * UEFI MemoryAllocationLib wrappers
 *
 *****************************************************************************
 */

VOID * EFIAPI AllocatePages ( IN UINTN Pages ) {
	return AllocateAlignedPages ( Pages, EFI_PAGE_SIZE );
}

VOID * EFIAPI AllocateRuntimePages ( IN UINTN Pages ) {
	return AllocateAlignedPages ( Pages, EFI_PAGE_SIZE );
}

VOID * EFIAPI AllocateReservedPages ( IN UINTN Pages ) {
	return AllocateAlignedPages ( Pages, EFI_PAGE_SIZE );
}

VOID EFIAPI FreePages ( IN VOID *Buffer, IN UINTN Pages ) {
	FreeAlignedPages ( Buffer, Pages );
}

VOID * EFIAPI AllocateAlignedPages ( IN UINTN Pages, IN UINTN Alignment ) {
	return PlatformAllocate ( Pages * EFI_PAGE_SIZE, Alignment );
}

VOID * EFIAPI AllocateAlignedRuntimePages ( IN UINTN Pages,
					    IN UINTN Alignment ) {
	return AllocateAlignedPages ( Pages, Alignment );
}

VOID * EFIAPI AllocateAlignedReservedPages ( IN UINTN Pages,
					     IN UINTN Alignment ) {
	return AllocateAlignedPages ( Pages, Alignment );
}

VOID EFIAPI FreeAlignedPages ( IN VOID *Buffer, IN UINTN Pages ) {
	PlatformFree ( Buffer );
	( VOID ) Pages;
}

VOID * EFIAPI AllocatePool ( IN UINTN AllocationSize ) {
	return PlatformAllocate ( AllocationSize, 0 );
}

VOID * EFIAPI AllocateRuntimePool ( IN UINTN AllocationSize ) {
	return AllocatePool ( AllocationSize );
}

VOID * EFIAPI AllocateReservedPool ( IN UINTN AllocationSize ) {
	return AllocatePool ( AllocationSize );
}

VOID * EFIAPI AllocateZeroPool ( IN UINTN AllocationSize ) {
	VOID *Data;

	Data = AllocatePool ( AllocationSize );
	if ( Data )
		ZeroMem ( Data, AllocationSize );
	return Data;
}

VOID * EFIAPI AllocateRuntimeZeroPool ( IN UINTN AllocationSize ) {
	return AllocateZeroPool ( AllocationSize );
}

VOID * EFIAPI AllocateReservedZeroPool ( IN UINTN AllocationSize ) {
	return AllocateZeroPool ( AllocationSize );
}

VOID * EFIAPI AllocateCopyPool ( IN UINTN AllocationSize,
				 IN CONST VOID *Buffer ) {
	VOID *Data;

	Data = AllocatePool ( AllocationSize );
	if ( Data )
		CopyMem ( Data, Buffer, AllocationSize );
	return Data;
}

VOID * EFIAPI AllocateRuntimeCopyPool ( IN UINTN AllocationSize,
					IN CONST VOID *Buffer ) {
	return AllocateCopyPool ( AllocationSize, Buffer );
}

VOID * EFIAPI AllocateReservedCopyPool ( IN UINTN AllocationSize,
					 IN CONST VOID *Buffer ) {
	return AllocateCopyPool ( AllocationSize, Buffer );
}

VOID * EFIAPI ReallocatePool ( IN UINTN OldSize, IN UINTN NewSize,
			       IN VOID *OldBuffer OPTIONAL ) {
	VOID *Data;

	Data = AllocatePool ( NewSize );
	if ( Data && OldBuffer ) {
		CopyMem ( Data, OldBuffer, MIN ( OldSize, NewSize ) );
		FreePool ( OldBuffer );
	}
	return Data;
}

VOID * EFIAPI ReallocateRuntimePool ( IN UINTN OldSize, IN UINTN NewSize,
				      IN VOID *OldBuffer OPTIONAL ) {
	return ReallocatePool ( OldSize, NewSize, OldBuffer );
}

VOID * EFIAPI ReallocateReservedPool ( IN UINTN OldSize, IN UINTN NewSize,
				       IN VOID *OldBuffer OPTIONAL ) {
	return ReallocatePool ( OldSize, NewSize, OldBuffer );
}

VOID EFIAPI FreePool ( IN VOID *Buffer ) {
	PlatformFree ( Buffer );
}
