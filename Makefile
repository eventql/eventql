build:
	cmake -DCMAKE_BUILD_TYPE=Release -Bbuild/workspace -H.
	cd build/workspace && make -j4
	@mkdir -p build/dist/bin
	@cp build/workspace/evqld build/dist/bin
	@cp build/workspace/evql build/dist/bin
	@cp src/eventql/install.sh build/dist
	@echo "BUILD SUCCESSFULL! -- output in build/dist"

install:
	@build/dist/install.sh

package:
	@./build/package.sh host

clean:
	rm -rf build/workspace build/dist build/package

.PHONY: build install clean
