all: libjs_static.a

libjs_static.a:
	echo "building mozjs..."
	mkdir -p out
	cd out && CC=$(CC) CXX=$(CXX) RANLIB=$(RANLIB) AR=$(AR) CFLAGS="-DJS_THREADSAFE" CXXFLAGS="-DJS_THREADSAFE" $(abs_top_srcdir)/deps/3rdparty/spidermonkey/mozjs/js/src/configure --disable-shared-js --disable-tests
	cd out && make JS_THREADSAFE=1
	cp out/dist/lib/libjs_static.a ./

EXTRA_DIST = distfiles.txt

dist-hook:
	builddir=`pwd`; cd $(srcdir) && cpio -pdum $$builddir/$(distdir) < distfiles.txt

