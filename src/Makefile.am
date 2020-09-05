@VALGRIND_CHECK_RULES@

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/edk2 \
	-I$(top_srcdir)/include/edk2/MdePkg/Include

AM_CFLAGS = \
	-W -Wall -Wextra -Werror \
	-ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables

AM_LDFLAGS = -no-undefined

bin_PROGRAMS = \
	efidevpath

lib_LTLIBRARIES = \
	libefikit.la

noinst_LTLIBRARIES = \
	libcommon.la \
	libmdebase.la \
	libmdebasedebugnull.la \
	libmdebasememory.la \
	libmdebaseprint.la \
	libmdeuefidevicepath.la

check_PROGRAMS = \
	efikittest

TESTS = $(check_PROGRAMS)

LOG_DRIVER = \
	env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh

export CMOCKA_MESSAGE_OUTPUT=TAP

###############################################################################
#
# EFI kit library
#
###############################################################################

libefikit_la_SOURCES = \
	libefidevpath.c \
	libefibootdev.c \
	efivars.c \
	efivars.h

libefikit_la_CPPFLAGS = \
	$(EFIVAR_CFLAGS) \
	$(AM_CPPFLAGS)

libefikit_la_LDFLAGS = \
	-export-symbols-regex '^(efidp|efiboot)_' \
	$(AM_LDFLAGS)

libefikit_la_LIBADD = \
	libcommon.la \
	libmdebase.la \
	libmdebasedebugnull.la \
	libmdebasememory.la \
	libmdebaseprint.la \
	libmdeuefidevicepath.la \
	$(LTLIBICONV) \
	$(EFIVAR_LIBS)

efikittest_SOURCES = \
	efikittest.c \
	efibootdevtest.c \
	efibootdevtest.h \
	efidevpathtest.c \
	efidevpathtest.h

efikittest_CPPFLAGS = \
	$(CMOCKA_CFLAGS) \
	$(AM_CPPFLAGS)

efikittest_CFLAGS = \
	-fshort-wchar \
	$(AM_CFLAGS)

efikittest_LDADD = \
	libefikit.la \
	$(CMOCKA_LIBS)

###############################################################################
#
# EFI device path command-line tool
#
###############################################################################

efidevpath_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(AM_CPPFLAGS)

efidevpath_LDADD = \
	libefikit.la \
	$(GLIB_LIBS)

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
	$(AM_CFLAGS) \
	-Wno-unused-parameter

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
	-D_PCD_GET_MODE_32_PcdMaximumAsciiStringLength=1000000 \
	-D_PCD_GET_MODE_32_PcdMaximumUnicodeStringLength=1000000 \
	$(AM_CFLAGS) \
	-Wno-implicit-fallthrough

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
	-D_PCD_GET_MODE_32_PcdMaximumDevicePathNodeCount=1000000 \
	$(AM_CFLAGS) \
	-Wno-unused-parameter
