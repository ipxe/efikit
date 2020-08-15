# Configure autoconf
AC_INIT([efikit], [0.0.1], [mbrown@fensystems.co.uk])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([include/edk2/ProcessorBind.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])

# Check for programs
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR

# Configure libtool
LT_INIT([shared pic-only disable-static])

# Configure automake
AM_INIT_AUTOMAKE([foreign subdir-objects tar-ustar])

# Check for libraries
PKG_CHECK_MODULES(GLIB, glib-2.0)

# Checks
AC_CHECK_HEADERS([])
AC_FUNC_MALLOC
AC_CHECK_FUNCS([])
AC_CHECK_DECLS([posix_memalign], [], [], [[#include <stdlib.h>]])
AC_CHECK_DECLS([_aligned_malloc], [], [], [[#include <malloc.h>]])
AM_ICONV

# Generate files
AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
