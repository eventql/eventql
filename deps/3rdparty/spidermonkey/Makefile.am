all-local: dist/lib/libjs_static.a

dist/lib/libjs_static.a:
	CC=$(CC) CXX=$(CXX) RANLIB=$(RANLIB) AR=$(AR) CFLAGS="-DJS_THREADSAFE" CXXFLAGS="-DJS_THREADSAFE" $(abs_top_srcdir)/deps/3rdparty/spidermonkey/mozjs/js/src/configure --disable-shared-js --disable-tests
	make JS_THREADSAFE=1
