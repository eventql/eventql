## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES =   \
  Makefile.in

EXTRA_DIST = sql_testdata

AM_CXXFLAGS = -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -Wno-invalid-offsetof -g -I@builddir@ -I@top_srcdir@/src -I@top_srcdir@/deps/3rdparty/protobuf

SIMDCOMP_CFLAGS = -I$(top_srcdir)/deps/3rdparty
SIMDCOMP_LDADD = ${top_builddir}/deps/3rdparty/libsimdcomp/libsimdcomp.a

PROTOBUF_CFLAGS = -I$(top_srcdir)/deps/3rdparty/protobuf
PROTOBUF_LDADD = ${top_builddir}/deps/3rdparty/protobuf/libprotobuf.a

LMDB_CFLAGS = -I$(top_srcdir)/deps/3rdparty
LMDB_LDADD = ${top_builddir}/deps/3rdparty/liblmdb/liblmdb.a

INIH_CFLAGS = -I$(top_srcdir)/deps/3rdparty
INIH_LDADD = ${top_builddir}/deps/3rdparty/inih/libinih.a


TESTS = \
  unit_metadata_file \
  unit_metadata_store \
  unit_metadata_operation_split \
  unit_metadata_operation_createpartition \
  unit_partition_discovery \
  unit_sql_parser \
  unit_sql_qtree

check_PROGRAMS = $(TESTS)

#smoketest1_SOURCES = smoketest1.cc
#smoketest1_LDADD = \
#    @top_builddir@/src/libevqlcore.la \
#    @top_builddir@/src/libevqlclient.la \
#    $(INIH_LDADD) \
#    $(SIMDCOMP_LDADD) \
#    $(LMDB_LDADD) \
#    $(PROTOBUF_LDADD)

unit_metadata_file_SOURCES = unit/metadata_file.cc
unit_metadata_file_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient.la \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

unit_metadata_store_SOURCES = unit/metadata_store.cc
unit_metadata_store_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient.la \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

unit_metadata_operation_split_SOURCES = unit/metadata_operation_split.cc
unit_metadata_operation_split_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient.la \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

unit_metadata_operation_createpartition_SOURCES = unit/metadata_operation_createpartition.cc
unit_metadata_operation_createpartition_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient.la \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

unit_partition_discovery_SOURCES = unit/partition_discovery.cc
unit_partition_discovery_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient.la \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

unit_sql_parser_SOURCES = unit/sql_parser.cc
unit_sql_parser_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient.la \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

unit_sql_qtree_SOURCES = unit/sql_qtree.cc
unit_sql_qtree_LDADD = \
    @top_builddir@/src/libevqlcore.a \
    @top_builddir@/src/libevqlclient.la \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD)

