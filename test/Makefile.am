## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES =   \
  Makefile.in

EXTRA_DIST = sql_testdata

AM_CXXFLAGS = -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wdelete-non-virtual-dtor -g -I@builddir@ -I@top_srcdir@/src -I@top_srcdir@/deps/3rdparty/protobuf

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

TESTS = \
  evql-test-unit-metadata_file \
  evql-test-unit-metadata_store \
  evql-test-unit-metadata_operation_split \
  evql-test-unit-metadata_operation_createpartition \
  evql-test-unit-partition_discovery \
  evql-test-unit-sql_parser \
  evql-test-unit-sql_qtree \
  evql-test-sql

check_PROGRAMS = $(TESTS)
noinst_PROGRAMS= evql-test

evql_test_SOURCES = evql_test.cc
evql_test_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_unit_metadata_file_SOURCES = unit/metadata_file.cc
evql_test_unit_metadata_file_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_unit_metadata_store_SOURCES = unit/metadata_store.cc
evql_test_unit_metadata_store_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_unit_metadata_operation_split_SOURCES = unit/metadata_operation_split.cc
evql_test_unit_metadata_operation_split_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_unit_metadata_operation_createpartition_SOURCES = unit/metadata_operation_createpartition.cc
evql_test_unit_metadata_operation_createpartition_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_unit_partition_discovery_SOURCES = unit/partition_discovery.cc
evql_test_unit_partition_discovery_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_unit_sql_parser_SOURCES = unit/sql_parser.cc
evql_test_unit_sql_parser_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_unit_sql_qtree_SOURCES = unit/sql_qtree.cc
evql_test_unit_sql_qtree_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

evql_test_sql_SOURCES = sql_test_runner.cc
evql_test_sql_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient_static.a \
    $(INIH_LDADD) \
    $(MURMURHASH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

