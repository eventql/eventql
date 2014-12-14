SHELL          = /bin/sh
CXX            = clang++
CXXFLAGS       = -stdlib=libc++ -I. -std=c++11
LDFLAGS        = 
TARGET         = cmapp
SOURCES        = main.cc tracker/tracker.cc

$(TARGET): $(SOURCES)
	cd fnord/base && make libfnord-base.a
	cd fnord/io && make libfnord-io.a
	cd fnord/net/http && make libfnord-http.a
	cd fnord/system && make libfnord-system.a
	cd fnord/thread && make libfnord-thread.a
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) -Lfnord/base -Lfnord/system -Lfnord/net/http -Lfnord/io -Lfnord/thread -lfnord-base -lfnord-system -lfnord-http -lfnord-io -lfnord-thread -lpthread

clean:
	rm -f $(TARGET)

.PHONY: all $(TARGET)
