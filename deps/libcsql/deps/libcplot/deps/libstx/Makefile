all: test

build:
	mkdir -p build/devel
	cd build/devel && \
	cmake -DCMAKE_BUILD_TYPE=Debug ../.. && \
	make

test: build
	/bin/bash -c 'cd build/devel && (ls -1 test-* | while read l; do ./$$l || exit 1; done)'

clean:
	rm -rf build

.PHONY: build test clean
