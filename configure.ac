## Process this file with autoconf to produce configure.
## In general, the safest way to proceed is to run ./autogen.sh

AC_PREREQ(2.59)
AC_INIT([EventQL],[0.5.0-dev],[authors@eventql.io],[eventql])
AM_INIT_AUTOMAKE([foreign subdir-objects serial-tests])
AM_MAINTAINER_MODE([enable])
AC_CONFIG_SRCDIR([configure.ac])
AM_SILENT_RULES([yes])

# --enable-mysql_tools
AC_MSG_CHECKING([Feature: MySQL Tools])
AC_ARG_ENABLE([mysql-tools], AS_HELP_STRING([--enable-mysql-tools], [Enable mysql tools]))
AM_CONDITIONAL([ENABLE_MYSQL_TOOLS], [test "x$enable_mysql_tools" = "xyes"])
AC_SUBST(ENABLE_MYSQL_TOOLS)
if test "x$enable_mysql_tools" = "xyes"; then
  AC_MSG_RESULT(["enabled"])
else
  AC_MSG_RESULT(["disabled"])
fi

# --enable-embedded-server
AC_MSG_CHECKING([Feature: Build Embedded Server: libevql_server.a])
AC_ARG_ENABLE([embedded-server], AS_HELP_STRING([--enable-embedded-server], [Enable embedded server]))
AM_CONDITIONAL([ENABLE_EMBEDDED_SERVER], [test "x$enable_embedded_server" = "xyes"])
AC_SUBST(ENABLE_EMBEDDED_SERVER)
if test "x$enable_embedded_server" = "xyes"; then
  AC_MSG_RESULT(["enabled"])
else
  AC_MSG_RESULT(["disabled"])
fi

# Check for build env
AC_PROG_CC
AC_PROG_CC_FOR_BUILD
AC_PROG_CXX
AC_PROG_CXX_FOR_BUILD
AM_PROG_AS
LT_INIT

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
AC_SUBST(EVQL_BUILDID)

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile deps/Makefile deps/3rdparty/protobuf/Makefile deps/3rdparty/linenoise/Makefile deps/3rdparty/libsimdcomp/Makefile deps/3rdparty/liblmdb/Makefile deps/3rdparty/spidermonkey/Makefile deps/3rdparty/zookeeper/Makefile deps/3rdparty/inih/Makefile build/osxpkg/distribution.xml build/osxpkg/package_info.xml])
AC_OUTPUT
