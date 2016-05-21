#!/bin/bash

PACKAGE=$1
VERSION=$2
PKG_DIR=${PKG_DIR:-.}

if [[ -z "$PACKAGE" || -z "$VERSION" ]]; then
  echo "usage: $0 <package> <version>" >&2
  exit 1
fi

if ! test -d $PACKAGE-$VERSION; then
  echo "ERROR: not found: $PACKAGE-$VERSION"
fi

# source packages
tar cz $PACKAGE-$VERSION > $PACKAGE-$VERSION.tgz

# binary packages
for triple in darwin_x86_64; do
  tar cz -C build/target/$PACKAGE-$VERSION-$triple/dist . \
      > $PACKAGE-$VERSION-$triple.tgz
done
