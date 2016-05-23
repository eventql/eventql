## Process this file with automake to produce Makefile.in.

ACLOCAL_AMFLAGS = -I m4

# Unit tests you want to run when people type 'make check'.
# TESTS is for binary unit tests, check_SCRIPTS for script-based tests
TESTS =
check_SCRIPTS =
dist_noinst_SCRIPTS =

# Some of the binaries we make are to be installed, and others are
# (non-user-visible) helper programs needed to build libpcre, libpcre16
# or libpcre32.
bin_PROGRAMS =
noinst_PROGRAMS =

# Additional files to delete on 'make clean' and 'make maintainer-clean'.
CLEANFILES =
MAINTAINERCLEANFILES =

# Additional files to bundle with the distribution, over and above what
# the Autotools include by default.
EXTRA_DIST =

# These files contain additional m4 macros that are used by autoconf.
EXTRA_DIST += \
  m4/ax_pthread.m4 m4/pcre_visibility.m4

# These files contain maintenance information
EXTRA_DIST += \
  NON-UNIX-USE \
  NON-AUTOTOOLS-BUILD \
  HACKING

# These files are used in the preparation of a release
EXTRA_DIST += \
  PrepareRelease \
  CheckMan \
  CleanTxt \
  Detrail \
  132html

# These files are to do with building for Virtual Pascal
EXTRA_DIST += \
  makevp.bat \
  makevp_c.txt \
  makevp_l.txt \
  pcregexp.pas


noinst_LIBRARIES = libpcre.a libpcreposix.a

libpcre_a_SOURCES = \
  config.h \
  pcre.h \
  pcre_byte_order.c \
  pcre_chartables.c \
  pcre_compile.c \
  pcre_config.c \
  pcre_dfa_exec.c \
  pcre_exec.c \
  pcre_fullinfo.c \
  pcre_get.c \
  pcre_globals.c \
  pcre_internal.h \
  pcre_jit_compile.c \
  pcre_maketables.c \
  pcre_newline.c \
  pcre_ord2utf8.c \
  pcre_refcount.c \
  pcre_string_utils.c \
  pcre_study.c \
  pcre_tables.c \
  pcre_ucd.c \
  pcre_valid_utf8.c \
  pcre_version.c \
  pcre_xclass.c \
  ucp.h

libpcre_a_CFLAGS = $(VISIBILITY_CFLAGS) $(AM_CFLAGS) -DHAVE_CONFIG_H=1
libpcre_a_LIBADD =

libpcreposix_a_SOURCES = pcreposix.c
libpcreposix_a_CFLAGS = $(VISIBILITY_CFLAGS) $(AM_CFLAGS) -DHAVE_CONFIG_H=1
libpcreposix_a_LIBADD = libpcre.a
