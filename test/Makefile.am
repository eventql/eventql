## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES =   \
  Makefile.in

AM_CXXFLAGS = -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -Wno-invalid-offsetof -g -I@builddir@ -I@top_srcdir@/src

SPIDERMONKEY_CFLAGS = -I$(top_builddir)/deps/3rdparty/spidermonkey/out/dist/include
SPIDERMONKEY_LDADD = ${top_builddir}/deps/3rdparty/spidermonkey/out/dist/lib/libjs_static.a

ZOOKEEPER_CFLAGS = -I$(top_srcdir)/deps/3rdparty/zookeeper/source/include -I$(top_srcdir)/deps/3rdparty/zookeeper/source/generated
ZOOKEEPER_LDADD = ${top_builddir}/deps/3rdparty/zookeeper/libzkmt.a ${top_builddir}/deps/3rdparty/zookeeper/libhashtable.a

PCRE_CFLAGS = -I$(top_srcdir)/deps/3rdparty/pcre -DHAVE_PCRE=1
PCRE_LDADD = ${top_builddir}/deps/3rdparty/pcre/libpcre.a

LINENOISE_CFLAGS = -I$(top_srcdir)/deps/3rdparty
LINENOISE_LDADD = ${top_builddir}/deps/3rdparty/linenoise/liblinenoise.a

SIMDCOMP_CFLAGS = -I$(top_srcdir)/deps/3rdparty
SIMDCOMP_LDADD = ${top_builddir}/deps/3rdparty/libsimdcomp/libsimdcomp.a

PROTOBUF_CFLAGS = -I$(top_srcdir)/deps/3rdparty/protobuf
PROTOBUF_LDADD = ${top_builddir}/deps/3rdparty/protobuf/libprotobuf.a

LMDB_CFLAGS = -I$(top_srcdir)/deps/3rdparty
LMDB_LDADD = ${top_builddir}/deps/3rdparty/liblmdb/liblmdb.a

INIH_CFLAGS = -I$(top_srcdir)/deps/3rdparty
INIH_LDADD = ${top_builddir}/deps/3rdparty/inih/libinih.a


check_PROGRAMS = smoketest1
TESTS = smoketest1

smoketest1_SOURCES = smoketest1.cc
smoketest1_LDADD = \
    @top_builddir@/src/libevqlcore.la \
    @top_builddir@/src/libevqlclient.la \
    $(ZOOKEEPER_LDADD) \
    $(LINENOISE_LDADD) \
    $(INIH_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD) \
    $(SPIDERMONKEY_LDADD) \
    $(PCRE_LDADD)



