#!/bin/bash
PACKAGE=$1
VERSION=$2

if [[ -z "$PACKAGE" || -z "$VERSION" ]]; then
  echo "usage: $0 <package> <version>" >&2
  exit 1
fi

if ! test -d $PACKAGE-$VERSION; then
  echo "ERROR: not found: $PACKAGE-$VERSION"
fi

TARGET_DIR=build/target/$PACKAGE-$VERSION-darwin_x86_64
test -d && rm -rf $TARGET_DIR
mkdir $TARGET_DIR $TARGET_DIR/dist
cd $TARGET_DIR
set -e

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

./configure --host=x86_64-apple-darwin14
make
make install DESTDIR=dist
