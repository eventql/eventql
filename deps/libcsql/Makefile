all: test

build:
	mkdir -p build/devel
	cd build/devel && \
	cmake -DCMAKE_BUILD_TYPE=Debug ../.. && \
	make

test: build
	/bin/bash -c 'ls -1 build/devel/test-chartsql-* | while read l; do ./$$l || exit 1; done'

clean:
	rm -rf build

.PHONY: build test clean
