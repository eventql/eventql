#!/bin/bash
set -e

PACKAGE=$1
VERSION=$2

if [[ -z "$PACKAGE" || -z "$VERSION" ]]; then
  echo "usage: $0 <package> <version>" >&2
  exit 1
fi

if ! test -d $DIST_SRC_DIR/$PACKAGE-$VERSION; then
  echo "ERROR: not found: $DIST_SRC_DIR/$PACKAGE-$VERSION"
fi

mkdir -p build/target;
mkdir -p build/dist-bin;

(
  TARGET_TRIPLE=x86_64-linux-gnu; \
  TARGET_DIR=build/target/$PACKAGE-$VERSION-$TARGET_TRIPLE; \
  DIST_DIR=build/dist-bin/$PACKAGE-$VERSION-$TARGET_TRIPLE; \
  rm -rf $TARGET_DIR $DIST_DIR; mkdir $TARGET_DIR $DIST_DIR; \
  cd $TARGET_DIR && \
  ../../dist-src/$PACKAGE-$VERSION/configure \
      --host=$TARGET_TRIPLE \
      CC="clang -target $TARGET_TRIPLE" \
      CXX="clang -target $TARGET_TRIPLE" && \
  make && \
  make install DESTDIR=../../dist-bin/$PACKAGE-$VERSION-$TARGET_TRIPLE
)

