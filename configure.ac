## Process this file with autoconf to produce configure.
## In general, the safest way to proceed is to run ./autogen.sh

AC_PREREQ(2.59)
AC_INIT([EventQL],[0.3.0],[authors@eventql.io],[eventql])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_MAINTAINER_MODE([enable])

AC_CONFIG_SRCDIR([configure.ac])

# Check for build env
AC_PROG_CC
AC_PROG_CC_FOR_BUILD
AC_PROG_CXX
AC_PROG_CXX_FOR_BUILD
AC_PROG_RANLIB
AM_PROG_AR
AM_PROG_AS

# Check for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdlib.h unistd.h syslog.h])
AM_CONDITIONAL([HAVE_SYSLOG_H], [test $HAVE_SYSLOG_H = 1])

# Check for library functions
AC_FUNC_MEMCMP
AC_FUNC_STRTOD
AC_CHECK_FUNCS([ftruncate memset mkdir strchr strerror strtol gethostbyname_r])
AM_CONDITIONAL([HAVE_GETHOSTBYNAME_R], [test $HAVE_GETHOSTBYNAME_R = 1])

# Check for zlib
HAVE_ZLIB=0
AS_IF([test "$with_zlib" != no], [
  AC_MSG_CHECKING([zlib version])

  # First check the zlib header version
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[
        #include <zlib.h>
        #if !defined(ZLIB_VERNUM) || (ZLIB_VERNUM < 0x1204)
        # error zlib version too old
        #endif
        ]], [])], [
    AC_MSG_RESULT([ok (1.2.0.4 or later)])

    # Also need to add -lz to the linker flags and make sure this succeeds.
    AC_SEARCH_LIBS([zlibVersion], [z], [
      AC_DEFINE([HAVE_ZLIB], [1], [Enable classes using zlib compression.])
      HAVE_ZLIB=1
    ], [
      AS_IF([test "$with_zlib" != check], [
        AC_MSG_FAILURE([--with-zlib was given, but no working zlib library was found])
      ])
    ])
  ], [
    AS_IF([test "$with_zlib" = check], [
      AC_MSG_RESULT([headers missing or too old (requires 1.2.0.4)])
    ], [
      AC_MSG_FAILURE([--with-zlib was given, but zlib headers were not present or were too old (requires 1.2.0.4)])
    ])
  ])
])
AM_CONDITIONAL([HAVE_ZLIB], [test $HAVE_ZLIB = 1])

# Check for pthread
ACX_PTHREAD
AM_CONDITIONAL([HAVE_PTHREAD], [test "x$acx_pthread_ok" = "xyes"])

# Pipe through mozjs cxx flags
AC_SUBST(MOZJS_CXXFLAGS)

AC_CONFIG_FILES([Makefile src/Makefile deps/3rdparty/protobuf/Makefile deps/3rdparty/linenoise/Makefile deps/3rdparty/libsimdcomp/Makefile deps/3rdparty/liblmdb/Makefile deps/3rdparty/spidermonkey/Makefile])
AC_OUTPUT
