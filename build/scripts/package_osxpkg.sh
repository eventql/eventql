#!/bin/bash

PACKAGE=$1
VERSION=$2
PKGDIR=${PKGDIR:-.}

if [[ -z "$PACKAGE" || -z "$VERSION" ]]; then
  echo "usage: $0 <package> <version>" >&2
  exit 1
fi

if ! test -d $PACKAGE-$VERSION; then
  echo "ERROR: not found: $PACKAGE-$VERSION"
fi

# binary packages
for triple in darwin_x86_64; do
  TMPDIR=build/target/$PACKAGE-$VERSION-$triple-osxpkg
  rm -rf $TMPDIR
  mkdir -p $TMPDIR $TMPDIR/flat/base.pkg $TMPDIR/flat/Resources/en.lproj $TMPDIR/root
  cp -r build/target/$PACKAGE-$VERSION-$triple/dist/* $TMPDIR/root
  cp build/osxpkg/distribution.xml $TMPDIR/flat/Distribution
  cp build/osxpkg/package_info.xml $TMPDIR/base.pkg/PackageInfo

  (cd $TMPDIR/root && find . | cpio -o --format odc --owner 0:80 | gzip -c) \
      > $TMPDIR/flat/base.pkg/Payload

  (cd $TMPDIR && mkbom -u 0 -g 80 root flat/base.pkg/Bom)

  (cd $TMPDIR/flat && xar --compression none -cf "$PKGDIR/$PACKAGE-$VERSION-$triple.pkg" *)
done
