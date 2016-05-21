#!/bin/bash

PACKAGE=$1
VERSION=$2
DIST_SRC_DIR=build/dist-src
PKG_DIR=${PKG_DIR:-.}

if [[ -z "$PACKAGE" || -z "$VERSION" ]]; then
  echo "usage: $0 <package> <version>" >&2
  exit 1
fi

if ! test -d $DIST_SRC_DIR/$PACKAGE-$VERSION; then
  echo "ERROR: not found: $DIST_SRC_DIR/$PACKAGE-$VERSION"
fi

# .tgz
tar cz -C $DIST_SRC_DIR $PACKAGE-$VERSION > $PKG_DIR/$PACKAGE-$VERSION.tgz

# .zip
(cd $DIST_SRC_DIR && zip -qr - $PACKAGE-$VERSION) > $PKG_DIR/$PACKAGE-$VERSION.zip

