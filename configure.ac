# Configure autoconf
AC_INIT([efikit], [0.2], [mbrown@fensystems.co.uk])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([include/edk2/ProcessorBind.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_CANONICAL_HOST

# Check for programs
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR

# Configure libtool
LT_INIT([shared pic-only disable-static win32-dll])

# Configure automake
AM_INIT_AUTOMAKE([foreign subdir-objects tar-ustar])

# Check for libraries
PKG_CHECK_MODULES(GLIB, glib-2.0)
PKG_CHECK_MODULES(CMOCKA, cmocka)

# Select EFI variable access mechanism
case "${host_os}" in
    linux*)
	PKG_CHECK_MODULES(EFIVAR, efivar)
	AC_DEFINE([EFIVAR_LIBEFIVAR], [1], [Use libefivar for variable access])
	;;
    windows*|mingw*)
	AC_DEFINE([EFIVAR_WINDOWS], [1], [Use Windows API for variable access])
	;;
    *)
	AC_DEFINE([EFIVAR_DUMMY], [1], [Use dummy variable access])
	AC_MSG_WARN(["No EFI variable access mechanism for ${host_os}"])
	;;
esac

# Checks
AC_CHECK_HEADERS([])
AC_CHECK_FUNCS([])
AC_CHECK_DECLS([posix_memalign], [], [], [[#include <stdlib.h>]])
AC_CHECK_DECLS([_aligned_malloc], [], [], [[#include <malloc.h>]])
AM_ICONV

# Linker flags
AX_CHECK_LINK_FLAG([-Wl,--gc-sections],
		   [AX_APPEND_FLAG([-Wl,--gc-sections], [LDFLAGS])],
		   [AX_APPEND_FLAG([-Wl,-dead_strip], [LDFLAGS])])

# Configure tests
AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_CHECK
AM_EXTRA_RECURSIVE_TARGETS([check-valgrind])
m4_foreach([vgtool], [valgrind_tool_list],
           [AM_EXTRA_RECURSIVE_TARGETS([check-valgrind-]vgtool)])

# Configure code coverage
AX_CODE_COVERAGE

# Generate files
AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
