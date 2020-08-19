AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/edk2 \
	-I$(top_srcdir)/include/edk2/MdePkg/Include

AM_CFLAGS = \
	-W -Wall -Wextra -Werror \
	-ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables \
	-Wl,--gc-sections

AM_LDFLAGS = -no-undefined

bin_PROGRAMS = \
	efidevpath

lib_LTLIBRARIES = \
	libefidevpath.la

noinst_LTLIBRARIES = \
	libcommon.la \
	libmdebase.la \
	libmdebasedebugnull.la \
	libmdebasememory.la \
	libmdebaseprint.la \
	libmdeuefidevicepath.la

check_PROGRAMS = \
	efidevpathtest

TESTS = $(check_PROGRAMS)

###############################################################################
#
# EFI device path library and command-line tool
#
###############################################################################

libefidevpath_la_SOURCES = \
	libefidevpath.c

libefidevpath_la_LDFLAGS = \
	-export-symbols-regex '^efidp_' \
	$(AM_LDFLAGS)

libefidevpath_la_LIBADD = \
	libcommon.la \
	libmdebase.la \
	libmdebasedebugnull.la \
	libmdebasememory.la \
	libmdebaseprint.la \
	libmdeuefidevicepath.la \
	$(LTLIBICONV)

efidevpath_SOURCES = \
	efidevpath.c

efidevpath_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(AM_CPPFLAGS)

efidevpath_LDADD = \
	libefidevpath.la \
	$(GLIB_LIBS)

efidevpathtest_SOURCES = \
	efidevpathtest.c

efidevpathtest_CPPFLAGS = \
	$(CMOCKA_CFLAGS) \
	$(AM_CPPFLAGS)

efidevpathtest_LDADD = \
	libefidevpath.la \
	$(CMOCKA_LIBS)

###############################################################################
#
# Common utility functions
#
###############################################################################

libcommon_la_SOURCES = \
	memalloc.c \
	strconvert.c \
	strconvert.h

###############################################################################
#
# edk2/MdePkg/Library/BaseLib
#
###############################################################################

libmdebase_la_SOURCES = \
	edk2/MdePkg/Library/BaseLib/ARShiftU64.c \
	edk2/MdePkg/Library/BaseLib/BaseLibInternals.h \
	edk2/MdePkg/Library/BaseLib/BitField.c \
	edk2/MdePkg/Library/BaseLib/CheckSum.c \
	edk2/MdePkg/Library/BaseLib/DivS64x64Remainder.c \
	edk2/MdePkg/Library/BaseLib/DivU64x32.c \
	edk2/MdePkg/Library/BaseLib/DivU64x32Remainder.c \
	edk2/MdePkg/Library/BaseLib/DivU64x64Remainder.c \
	edk2/MdePkg/Library/BaseLib/FilePaths.c \
	edk2/MdePkg/Library/BaseLib/GetPowerOfTwo32.c \
	edk2/MdePkg/Library/BaseLib/GetPowerOfTwo64.c \
	edk2/MdePkg/Library/BaseLib/HighBitSet32.c \
	edk2/MdePkg/Library/BaseLib/HighBitSet64.c \
	edk2/MdePkg/Library/BaseLib/LinkedList.c \
	edk2/MdePkg/Library/BaseLib/LongJump.c \
	edk2/MdePkg/Library/BaseLib/LowBitSet32.c \
	edk2/MdePkg/Library/BaseLib/LowBitSet64.c \
	edk2/MdePkg/Library/BaseLib/LRotU32.c \
	edk2/MdePkg/Library/BaseLib/LRotU64.c \
	edk2/MdePkg/Library/BaseLib/LShiftU64.c \
	edk2/MdePkg/Library/BaseLib/Math64.c \
	edk2/MdePkg/Library/BaseLib/ModU64x32.c \
	edk2/MdePkg/Library/BaseLib/MultS64x64.c \
	edk2/MdePkg/Library/BaseLib/MultU64x32.c \
	edk2/MdePkg/Library/BaseLib/MultU64x64.c \
	edk2/MdePkg/Library/BaseLib/RRotU32.c \
	edk2/MdePkg/Library/BaseLib/RRotU64.c \
	edk2/MdePkg/Library/BaseLib/RShiftU64.c \
	edk2/MdePkg/Library/BaseLib/SafeString.c \
	edk2/MdePkg/Library/BaseLib/SetJump.c \
	edk2/MdePkg/Library/BaseLib/String.c \
	edk2/MdePkg/Library/BaseLib/SwapBytes16.c \
	edk2/MdePkg/Library/BaseLib/SwapBytes32.c \
	edk2/MdePkg/Library/BaseLib/SwapBytes64.c \
	edk2/MdePkg/Library/BaseLib/SwitchStack.c \
	edk2/MdePkg/Library/BaseLib/Unaligned.c

libmdebase_la_CFLAGS = \
	-fshort-wchar \
	-include Uefi/UefiBaseType.h \
	-D_PCD_GET_MODE_BOOL_PcdVerifyNodeInList=FALSE \
	-D_PCD_GET_MODE_32_PcdMaximumLinkedListLength=1000000 \
	-D_PCD_GET_MODE_32_PcdMaximumAsciiStringLength=1000000 \
	-D_PCD_GET_MODE_32_PcdMaximumUnicodeStringLength=1000000 \
	$(AM_CFLAGS)

###############################################################################
#
# edk2/MdePkg/Library/BaseDebugLibNull
#
###############################################################################

libmdebasedebugnull_la_SOURCES = \
	edk2/MdePkg/Library/BaseDebugLibNull/DebugLib.c

libmdebasedebugnull_la_CFLAGS = \
	-Wno-unused-parameter \
	$(AM_CFLAGS)

###############################################################################
#
# edk2/MdePkg/Library/BaseMemoryLib
#
###############################################################################

libmdebasememory_la_SOURCES = \
	edk2/MdePkg/Library/BaseMemoryLib/CompareMemWrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/CopyMem.c \
	edk2/MdePkg/Library/BaseMemoryLib/CopyMemWrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/IsZeroBufferWrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/MemLibGeneric.c \
	edk2/MdePkg/Library/BaseMemoryLib/MemLibGuid.c \
	edk2/MdePkg/Library/BaseMemoryLib/MemLibInternals.h \
	edk2/MdePkg/Library/BaseMemoryLib/ScanMem16Wrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/ScanMem32Wrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/ScanMem64Wrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/ScanMem8Wrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/SetMem16Wrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/SetMem32Wrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/SetMem64Wrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/SetMem.c \
	edk2/MdePkg/Library/BaseMemoryLib/SetMemWrapper.c \
	edk2/MdePkg/Library/BaseMemoryLib/ZeroMemWrapper.c

###############################################################################
#
# edk2/MdePkg/Library/BasePrintLib
#
###############################################################################

libmdebaseprint_la_SOURCES = \
	edk2/MdePkg/Library/BasePrintLib/PrintLib.c \
	edk2/MdePkg/Library/BasePrintLib/PrintLibInternal.c \
	edk2/MdePkg/Library/BasePrintLib/PrintLibInternal.h

libmdebaseprint_la_CFLAGS = \
	-Wno-implicit-fallthrough \
	-D_PCD_GET_MODE_32_PcdMaximumAsciiStringLength=1000000 \
	-D_PCD_GET_MODE_32_PcdMaximumUnicodeStringLength=1000000 \
	$(AM_CFLAGS)

###############################################################################
#
# edk2/MdePkg/Library/UefiDevicePathLib
#
###############################################################################

libmdeuefidevicepath_la_SOURCES = \
	edk2/MdePkg/Library/UefiDevicePathLib/DevicePathFromText.c \
	edk2/MdePkg/Library/UefiDevicePathLib/DevicePathToText.c \
	edk2/MdePkg/Library/UefiDevicePathLib/DevicePathUtilities.c \
	edk2/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.c \
	edk2/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.h

libmdeuefidevicepath_la_CFLAGS = \
	-fshort-wchar \
	-Wno-unused-parameter \
	-D_PCD_GET_MODE_32_PcdMaximumDevicePathNodeCount=1000000 \
	$(AM_CFLAGS)
