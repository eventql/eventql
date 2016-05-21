#!/bin/bash
PACKAGE=$1
VERSION=$2

if [[ -z "$PACKAGE" || -z "$VERSION" ]]; then
  echo "usage: $0 <package> <version>" >&2
  exit 1
fi

if ! test -d $DIST_SRC_DIR/$PACKAGE-$VERSION; then
  echo "ERROR: not found: $DIST_SRC_DIR/$PACKAGE-$VERSION"
fi

TARGET_DIR=build/target/$PACKAGE-$VERSION-darwin_x86_64
DIST_DIR=build/dist-bin/$PACKAGE-$VERSION-darwin_x86_64

mkdir -p build/target;
mkdir -p build/dist-bin;
rm -rf $TARGET_DIR $DIST_DIR
mkdir $TARGET_DIR $DIST_DIR
cd $TARGET_DIR
set -e

../../dist-src/$PACKAGE-$VERSION/configure \
    --host=x86_64-apple-darwin14 \
    CC="x86_64-apple-darwin14-cc" \
    CXX="x86_64-apple-darwin14-c++" \
    MOZJS_CXXFLAGS="-DXP_MACOSX=1"

make
make install DESTDIR=../../dist-bin/$PACKAGE-$VERSION-darwin_x86_64

