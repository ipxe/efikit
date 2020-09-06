CODE_COVERAGE_BRANCH_COVERAGE = 1

include $(top_srcdir)/aminclude_static.am

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src include

EXTRA_DIST = autogen.sh README.md

clean-local: code-coverage-clean

distclean-local: code-coverage-dist-clean
