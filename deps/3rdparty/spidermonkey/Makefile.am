all: libjs_static.a

libjs_static.a:
	echo "building mozjs..."
	mkdir -p out
	cd out && CC=$(CC) CXX=$(CXX) RANLIB=$(RANLIB) AR=$(AR) CFLAGS="-DJS_THREADSAFE" CXXFLAGS="-DJS_THREADSAFE $(MOZJS_CXXFLAGS)" $(abs_top_srcdir)/deps/3rdparty/spidermonkey/mozjs/js/src/configure --disable-shared-js --disable-tests --without-intl-api --build=$(build_triplet) --host=$(host_triplet)
	mkdir -p out/js/src/host
	cp $(srcdir)/jsautokw.h out/js/src/
	cd out && make JS_THREADSAFE=1
	cp out/dist/lib/libjs_static.a ./

EXTRA_DIST = distfiles.txt

dist-hook:
	builddir=`pwd`; cd $(srcdir) && cpio -pdum $$builddir/$(distdir) < distfiles.txt

