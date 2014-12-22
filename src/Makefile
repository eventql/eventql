SHELL          = /bin/sh
CXX            = clang++
CXXFLAGS       = -I. -std=c++11
LDFLAGS        = 
TARGET         = cmapp
SOURCES        = main.cc itemref.cc customernamespace.cc tracker/tracker.cc tracker/logjoinservice.cc tracker/logjoinoutput.cc tracker/trackedsession.cc tracker/trackedquery.cc

UNAME = $(shell uname)
ifeq ($(UNAME), Darwin)
CXXFLAGS += -stdlib=libc++
endif

$(TARGET): $(SOURCES)
	cd fnord/base && make libfnord-base.a
	cd fnord/io && make libfnord-io.a
	cd fnord/logging && make libfnord-logging.a
	cd fnord/net && make libfnord-net.a
	cd fnord/net/http && make libfnord-http.a
	cd fnord/system && make libfnord-system.a
	cd fnord/thread && make libfnord-thread.a
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) -Lfnord/base -Lfnord/system -Lfnord/net/http -Lfnord/io -Lfnord/logging -Lfnord/net -Lfnord/thread -lfnord-system -lfnord-http -lfnord-io -lfnord-logging -lfnord-net -lfnord-thread -lfnord-base -lpthread

clean:
	cd fnord && make clean
	rm -f $(TARGET)

.PHONY: all $(TARGET)
