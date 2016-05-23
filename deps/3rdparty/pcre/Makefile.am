## Process this file with automake to produce Makefile.in.

ACLOCAL_AMFLAGS = -I m4

dist_doc_DATA = \
  doc/pcre.txt \
  doc/pcre-config.txt \
  doc/pcregrep.txt \
  doc/pcretest.txt \
  AUTHORS \
  COPYING \
  ChangeLog \
  LICENCE \
  NEWS \
  README


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
  doc/perltest.txt \
  NON-UNIX-USE \
  NON-AUTOTOOLS-BUILD \
  HACKING

# These files are used in the preparation of a release
EXTRA_DIST += \
  PrepareRelease \
  CheckMan \
  CleanTxt \
  Detrail \
  132html \
  doc/index.html.src

# These files are to do with building for Virtual Pascal
EXTRA_DIST += \
  makevp.bat \
  makevp_c.txt \
  makevp_l.txt \
  pcregexp.pas

# These files are usable versions of pcre.h and config.h that are distributed
# for the benefit of people who are building PCRE manually, without the
# Autotools support.
EXTRA_DIST += \
  pcre.h.generic \
  config.h.generic

# The only difference between pcre.h.in and pcre.h is the setting of the PCRE
# version number. Therefore, we can create the generic version just by copying.
pcre.h.generic: pcre.h.in configure.ac
	rm -f $@
	cp -p pcre.h $@

# It is more complicated for config.h.generic. We need the version that results
# from a default configuration so as to get all the default values for PCRE
# configuration macros such as MATCH_LIMIT and NEWLINE. We can get this by
# doing a configure in a temporary directory. However, some trickery is needed,
# because the source directory may already be configured. If you just try
# running configure in a new directory, it complains. For this reason, we move
# config.status out of the way while doing the default configuration. The
# resulting config.h is munged by perl to put #ifdefs round any #defines for
# macros with values, and to #undef all boolean macros such as HAVE_xxx and
# SUPPORT_xxx. We also get rid of any gcc-specific visibility settings. Make
# sure that PCRE_EXP_DEFN is unset (in case it has visibility settings).
config.h.generic: configure.ac
	rm -rf $@ _generic
	mkdir _generic
	cs=$(srcdir)/config.status; test ! -f $$cs || mv -f $$cs $$cs.aside
	cd _generic && $(abs_top_srcdir)/configure || :
	cs=$(srcdir)/config.status; test ! -f $$cs.aside || mv -f $$cs.aside $$cs
	test -f _generic/config.h
	perl -n \
	  -e 'BEGIN{$$blank=0;}' \
	  -e 'if(/PCRE_EXP_DEFN/){print"/* #undef PCRE_EXP_DEFN */\n";$$blank=0;next;}' \
	  -e 'if(/to make a symbol visible/){next;}' \
	  -e 'if(/__attribute__ \(\(visibility/){next;}' \
	  -e 'if(/LT_OBJDIR/){print"/* This is ignored unless you are using libtool. */\n";}' \
	  -e 'if(/^#define\s((?:HAVE|SUPPORT|STDC)_\w+)/){print"/* #undef $$1 */\n";$$blank=0;next;}' \
	  -e 'if(/^#define\s(?!PACKAGE|VERSION)(\w+)/){print"#ifndef $$1\n$$_#endif\n";$$blank=0;next;}' \
	  -e 'if(/^\s*$$/){print unless $$blank; $$blank=1;} else{print;$$blank=0;}' \
	  _generic/config.h >$@
	rm -rf _generic

MAINTAINERCLEANFILES += pcre.h.generic config.h.generic

# These are the header files we'll install. We do not distribute pcre.h because
# it is generated from pcre.h.in.
nodist_include_HEADERS = pcre.h
include_HEADERS = pcreposix.h

noinst_LIBRARIES = libpcre.a libpcreposix.a

libpcre_a_SOURCES = \
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
libpcre_a_LDFLAGS = $(EXTRA_LIBPCRE_LDFLAGS)

libpcreposix_a_SOURCES = pcreposix.c
libpcreposix_a_CFLAGS = $(VISIBILITY_CFLAGS) $(AM_CFLAGS) -DHAVE_CONFIG_H=1
libpcreposix_a_LDFLAGS = $(EXTRA_LIBPCREPOSIX_LDFLAGS)
libpcreposix_a_LIBADD = libpcre.a
