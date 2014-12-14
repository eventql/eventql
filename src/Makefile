SHELL          = /bin/sh
CXX           ?= clang++
CXXFLAGS       = -stdlib=libc++ -I. -std=c++11
LDFLAGS        = -Lfnord/base -Lfnord/system -Lfnord/net/http -Lfnord/io -Lfnord/thread -lfnord-base -lfnord-system -lfnord-http -lfnord-io -lfnord-thread -lpthread

TARGET         = cmapp
SOURCES        = main.cc tracker/tracker.cc

$(TARGET): $(SOURCES) fnord/base/libfnord-base.a fnord/net/http/libfnord-http.a fnord/system/libfnord-system.a fnord/io/libfnord-io.a fnord/thread/libfnord-thread.a
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

fnord/base/libfnord-base.a:
	cd fnord/base && make libfnord-base.a

fnord/io/libfnord-io.a:
	cd fnord/io && make libfnord-io.a

fnord/net/http/libfnord-http.a:
	cd fnord/net/http && make libfnord-http.a

fnord/system/libfnord-system.a:
	cd fnord/system && make libfnord-system.a

fnord/thread/libfnord-thread.a:
	cd fnord/thread && make libfnord-thread.a

clean:
	rm -f $(TARGET)

.PHONY: all
