all: libzkmt.a

libzkmt.a:
	mkdir -p out
	cd out && CC=$(CC) CXX=$(CXX) RANLIB=$(RANLIB) AR=$(AR) CFLAGS="-DJS_THREADSAFE" CXXFLAGS="-DJS_THREADSAFE $(MOZJS_CXXFLAGS)" $(abs_top_srcdir)/deps/3rdparty/zookeeper/source/configure --disable-shared --build=$(build_triplet) --host=$(host_triplet)
	cd out && make
	cp out/.libs/libzkmt.a ./

EXTRA_DIST =

BUILT_SOURCES = libzkmt.a


