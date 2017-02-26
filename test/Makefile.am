## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES =   \
  Makefile.in

EXTRA_DIST = sql_testdata

AM_CXXFLAGS = -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wdelete-non-virtual-dtor -g -I@builddir@ -I@top_srcdir@/src -I@top_srcdir@/deps/3rdparty/protobuf -lpthread

SIMDCOMP_CFLAGS = -I$(top_srcdir)/deps/3rdparty
SIMDCOMP_LDADD = ${top_builddir}/deps/3rdparty/libsimdcomp/libsimdcomp.a

PROTOBUF_CFLAGS = -I$(top_srcdir)/deps/3rdparty/protobuf
PROTOBUF_LDADD = ${top_builddir}/deps/3rdparty/protobuf/libprotobuf.a

LMDB_CFLAGS = -I$(top_srcdir)/deps/3rdparty
LMDB_LDADD = ${top_builddir}/deps/3rdparty/liblmdb/liblmdb.a

INIH_CFLAGS = -I$(top_srcdir)/deps/3rdparty
INIH_LDADD = ${top_builddir}/deps/3rdparty/inih/libinih.a

MURMURHASH_CFLAGS = -I$(top_srcdir)/deps/3rdparty
MURMURHASH_LDADD = ${top_builddir}/deps/3rdparty/murmurhash/libmurmurhash.a

noinst_PROGRAMS= evql-test

evql_test_SOURCES = \
    evql_test.cc \
    automate/process.h \
    automate/process.cc \
    unit_tests.cc \
    unit/metadata_store.cc \
    unit/metadata_file.cc \
    unit/metadata_operation_createpartition.cc \
    unit/metadata_operation_split.cc \
    unit/partition_discovery.cc \
    unit/sql_parser.cc \
    unit/sql_qtree.cc \
    system/basic_sql/basic_sql.h \
    system/basic_sql/basic_sql.cc \
    sql_tests.cc \
    test_repository.cc \
    test_runner.cc

evql_test_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

