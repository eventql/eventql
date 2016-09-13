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

TARGET_DIR=build/target/$PACKAGE-$VERSION-darwin_x86_64
test -d $TARGET_DIR/dist && rm -rf $TARGET_DIR/dist || true
mkdir -p $TARGET_DIR $TARGET_DIR/dist || true
cd $TARGET_DIR

export CC=x86_64-apple-darwin14-cc
export CXX=x86_64-apple-darwin14-c++
export LD=x86_64-apple-darwin14-ld
export AR=x86_64-apple-darwin14-ar
export AS=x86_64-apple-darwin14-as
export NM=x86_64-apple-darwin14-nm
export STRIP=x86_64-apple-darwin14-strip
export RANLIB=x86_64-apple-darwin14-ranlib
export OBJDUMP=x86_64-apple-darwin14-objdump
export CXXFLAGS="-stdlib=libc++"
export MOZJS_CXXFLAGS="-DXP_MACOSX=1"

../../../$PACKAGE-$VERSION/configure --host=x86_64-apple-darwin14 --prefix=/usr/local
#(cd src && make clean)
(cd deps && make)
make
make install DESTDIR=$(pwd)/dist
