## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES =   \
  Makefile.in

AM_CXXFLAGS =  -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -Wno-invalid-offsetof -g -I@builddir@ -I@top_srcdir@/src

check_PROGRAMS = smoketest1

smoketest1_SOURCES = smoketest1.cc
smoketest1_LDADD = @top_builddir@/src/evql_util.a


