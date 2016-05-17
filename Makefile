build:
	BUILD_TYPE=debug BUILD_ARTIFACTS=false RUN_TESTS=false ./build.sh devel

test:
	BUILD_TYPE=debug BUILD_ARTIFACTS=false RUN_TESTS=true ./build.sh devel

clean:
	rm -rf build

.PHONY: build test clean
