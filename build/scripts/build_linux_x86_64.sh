#!/bin/bash
PACKAGE=$1
VERSION=$2
set -e

if [[ -z "$PACKAGE" || -z "$VERSION" ]]; then
  echo "usage: $0 <package> <version>" >&2
  exit 1
fi

if ! test -d $PACKAGE-$VERSION; then
  echo "ERROR: not found: $PACKAGE-$VERSION"
fi

TARGET_DIR=build/target/$PACKAGE-$VERSION-linux_x86_64
test -d $TARGET_DIR/dist && rm -rf $TARGET_DIR/dist || true
mkdir -p $TARGET_DIR $TARGET_DIR/dist || true
cd $TARGET_DIR

export CC=x86_64-linux-gnu-gcc
export CXX=x86_64-linux-gnu-g++
export LD=x86_64-linux-gnu-ld
export AR=x86_64-linux-gnu-ar
export AS=x86_64-linux-gnu-as
export NM=x86_64-linux-gnu-nm
export STRIP=x86_64-linux-gnu-strip
export RANLIB=x86_64-linux-gnu-ranlib
export OBJDUMP=x86_64-linux-gnu-objdump
export CXXFLAGS="-static-libstdc++ -static-libgcc"

../../../$PACKAGE-$VERSION/configure --host=x86_64-linux-gnu --prefix=/usr/local
#(cd src && make clean)
(cd deps && make)
make
make install DESTDIR=$(pwd)/dist
