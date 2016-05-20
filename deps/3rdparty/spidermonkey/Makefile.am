all: libjs_static.a

libjs_static.a:
	echo "building mozjs..."
	mkdir -p tmp
	cd tmp && CC=$(CC) CXX=$(CXX) RANLIB=$(RANLIB) AR=$(AR) CFLAGS="-DJS_THREADSAFE" CXXFLAGS="-DJS_THREADSAFE" $(abs_top_srcdir)/deps/3rdparty/spidermonkey/mozjs/js/src/configure --disable-shared-js --disable-tests
	cd tmp && make JS_THREADSAFE=1
	cp tmp/dist/lib/libjs_static.a ./

EXTRA_DIST = distfiles.txt

dist-hook:
	builddir=`pwd`; cd $(srcdir) && cpio -pdum $$builddir/$(distdir) < distfiles.txt

