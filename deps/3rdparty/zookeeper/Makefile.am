all: libzkmt.a

libzkmt.a libhashtable.a:
	mkdir -p out
	cd out && CC=$(CC) CXX=$(CXX) RANLIB=$(RANLIB) AR=$(AR) CFLAGS="-DJS_THREADSAFE" CXXFLAGS="-DJS_THREADSAFE $(MOZJS_CXXFLAGS)" $(abs_top_srcdir)/deps/3rdparty/zookeeper/source/configure --disable-shared --build=$(build_triplet) --host=$(host_triplet)
	cd out && make
	cp out/.libs/libzkmt.a ./
	cp out/.libs/libhashtable.a ./

BUILT_SOURCES = libzkmt.a libhashtable.a

EXTRA_DIST = \
    source \
    source/acinclude.m4 \
    source/aclocal.m4 \
    source/aminclude.am \
    source/autom4te.cache \
    source/autom4te.cache/output.0 \
    source/autom4te.cache/output.1 \
    source/autom4te.cache/requests \
    source/autom4te.cache/traces.0 \
    source/autom4te.cache/traces.1 \
    source/c-doc.Doxyfile \
    source/ChangeLog \
    source/Cli.vcproj \
    source/compile \
    source/config.guess \
    source/config.h.in \
    source/config.sub \
    source/configure \
    source/configure.ac \
    source/depcomp \
    source/generated \
    source/generated/zookeeper.jute.c \
    source/generated/zookeeper.jute.h \
    source/include \
    source/include/proto.h \
    source/include/recordio.h \
    source/include/winconfig.h \
    source/include/winstdint.h \
    source/include/zookeeper.h \
    source/include/zookeeper_log.h \
    source/include/zookeeper_version.h \
    source/INSTALL \
    source/install-sh \
    source/LICENSE \
    source/ltmain.sh \
    source/Makefile.am \
    source/Makefile.in \
    source/missing \
    source/NOTICE.txt \
    source/README \
    source/src \
    source/src/cli.c \
    source/src/hashtable \
    source/src/hashtable/hashtable.c \
    source/src/hashtable/hashtable.h \
    source/src/hashtable/hashtable_itr.c \
    source/src/hashtable/hashtable_itr.h \
    source/src/hashtable/hashtable_private.h \
    source/src/hashtable/LICENSE.txt \
    source/src/load_gen.c \
    source/src/mt_adaptor.c \
    source/src/recordio.c \
    source/src/st_adaptor.c \
    source/src/winport.c \
    source/src/winport.h \
    source/src/zk_adaptor.h \
    source/src/zk_hashtable.c \
    source/src/zk_hashtable.h \
    source/src/zk_log.c \
    source/src/zookeeper.c \
    source/tests \
    source/tests/CollectionUtil.h \
    source/tests/CppAssertHelper.h \
    source/tests/LibCMocks.cc \
    source/tests/LibCMocks.h \
    source/tests/LibCSymTable.cc \
    source/tests/LibCSymTable.h \
    source/tests/MocksBase.cc \
    source/tests/MocksBase.h \
    source/tests/PthreadMocks.cc \
    source/tests/PthreadMocks.h \
    source/tests/TestClient.cc \
    source/tests/TestClientRetry.cc \
    source/tests/TestDriver.cc \
    source/tests/TestMulti.cc \
    source/tests/TestOperations.cc \
    source/tests/TestWatchers.cc \
    source/tests/TestZookeeperClose.cc \
    source/tests/TestZookeeperInit.cc \
    source/tests/ThreadingUtil.cc \
    source/tests/ThreadingUtil.h \
    source/tests/Util.cc \
    source/tests/Util.h \
    source/tests/Vector.h \
    source/tests/wrappers-mt.opt \
    source/tests/wrappers.opt \
    source/tests/ZKMocks.cc \
    source/tests/ZKMocks.h \
    source/tests/zkServer.sh \
    source/zookeeper.sln \
    source/zookeeper.vcproj
