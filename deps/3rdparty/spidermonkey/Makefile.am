all: libjs_static.a

libjs_static.a:
	mkdir -p out/js/src/host
	touch $(abs_top_srcdir)/deps/3rdparty/spidermonkey/mozjs/js/src/configure
	cd out && CC=$(CC) CXX=$(CXX) RANLIB=$(RANLIB) AR=$(AR) CFLAGS="-DJS_THREADSAFE" CXXFLAGS="-DJS_THREADSAFE $(MOZJS_CXXFLAGS)" $(abs_top_srcdir)/deps/3rdparty/spidermonkey/mozjs/js/src/configure --disable-shared-js --disable-tests --without-intl-api --build=$(build_triplet) --host=$(host_triplet)
	cp $(srcdir)/jsautokw.h out/js/src/
	cd out && make JS_THREADSAFE=1
	cp out/dist/lib/libjs_static.a ./

EXTRA_DIST = distfiles.txt jsautokw.h

dist-hook:
	builddir=`pwd`; cd $(srcdir) && cpio -pdum $$builddir/$(distdir) < distfiles.txt

