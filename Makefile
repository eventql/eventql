debug:
	BUILD_TYPE=debug BUILD_ASSETS=false BUILD_DOCUMENTATION=false BUILD_ARTIFACTS=false RUN_TESTS=false ./build.sh devel

test:
	BUILD_TYPE=debug BUILD_ASSETS=false BUILD_DOCUMENTATION=false BUILD_ARTIFACTS=false ./build.sh devel

release:
	BUILD_TYPE=release RUN_TESTS=false ./build.sh release

clean:
	rm -rf build

.PHONY: clean release debug test
