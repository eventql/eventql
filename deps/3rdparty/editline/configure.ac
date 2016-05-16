# Minix libeditline                                           -*- Autoconf -*-
#
# Copyright (c) 2008-2013  Joachim Nilsson <troglobit()gmail!com>
#
# Process this file with autoconf to produce a configure script.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_PREREQ(2.61)
AC_INIT(editline, 1.15.0, https://github.com/troglobit/editline/issues)
AM_INIT_AUTOMAKE([no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/editline.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
LT_INIT

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_STDC
# Check for malloc.h instead of AC_FUNC_MALLOC/REALLOC AIX and others
# mess up the traditional malloc check.
AC_CHECK_HEADERS([malloc.h signal.h stdlib.h string.h termcap.h termio.h termios.h sgtty.h])

# In termios.h or in sys/ioctl.g?
AC_HEADER_TIOCGWINSZ

# Overrides and types, should be a check.
AC_DEFINE([SYS_UNIX], [1], [Default to UNIX backend, should be detected.])
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
AC_FUNC_STAT
AC_CHECK_FUNCS([strchr strdup strrchr tcgetattr perror])

AC_ARG_ENABLE(unique-history,
   [AS_HELP_STRING([--disable-unique-history],
                   [Disable uniqify of scrollback. Default: duplicate entries are ignored. Use this to save dupes.])],
   , AC_DEFINE(CONFIG_UNIQUE_HISTORY, 1, [Define to skip duplicate lines in the scrollback history.]))

AC_ARG_ENABLE(default-complete,
   [AS_HELP_STRING([--disable-default-complete], [Disable default (filename) completion handler.])],
   , AC_DEFINE(CONFIG_DEFAULT_COMPLETE, 1, [Define to enable the default completion handler.]))

AC_ARG_ENABLE(arrow-keys,
   [AS_HELP_STRING([--disable-arrow-keys], [Disable ANSI arrow keys.])],
   , AC_DEFINE(CONFIG_ANSI_ARROWS, 1, [Define to include ANSI arrow keys support.]))

AC_ARG_ENABLE(eof,
   [AS_HELP_STRING([--disable-eof], [Disable default EOF (Ctrl-D) behavior.])],
   , AC_DEFINE([CONFIG_EOF], 1, [Define to enable EOF (Ctrl-C) key.]))

AC_ARG_ENABLE(sigint,
   [AS_HELP_STRING([--disable-sigint], [Disable default SIGINT (Ctrl-C) behavior.])],
   , AC_DEFINE([CONFIG_SIGINT], 1, [Define to enable SIGINT (Ctrl-C) key.]))

AC_ARG_ENABLE(sigstop,
   [AS_HELP_STRING([--enable-sigstop], [Enable SIGSTOP (Ctrl-Z) behavior.])],
   AC_DEFINE([CONFIG_SIGSTOP], 1, [Define to enable SIGSTOP (Ctrl-Z) key.]))

AC_ARG_ENABLE(terminal-bell,
   [AS_HELP_STRING([--enable-terminal-bell], [Enable terminal bell on completion.])],
   AC_DEFINE([CONFIG_TERMINAL_BELL], 1, [Define to enable terminal bell on completion.]))

AC_ARG_ENABLE(termcap,
   [AS_HELP_STRING([--enable-termcap], [Use termcap library to query terminal size.])],
   AC_DEFINE([CONFIG_USE_TERMCAP], 1, [Define to use the termcap library for terminal size.]))

AM_CFLAGS="-std=gnu99 $inline_cflags -W -Wall -Wundef -Wunused -Wstrict-prototypes -Werror-implicit-function-declaration $nopointersign_cflags -Wshadow -Wcast-qual"
AC_SUBST(AM_CFLAGS)

AC_OUTPUT
