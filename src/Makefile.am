AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/edk2 \
	-I$(top_srcdir)/include/edk2/MdePkg/Include

AM_CFLAGS = \
	-W -Wall -Wextra -Werror \
	-ffunction-sections -fdata-sections -Wl,--gc-sections

noinst_LTLIBRARIES = \
	libmdebase.la \
	libmdebasememory.la \
	libmdebaseprint.la

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
