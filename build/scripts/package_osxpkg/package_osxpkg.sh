#!/bin/bash
set -e

mkdir -p build/workspace/osxpkg
mkdir -p build/package

pkgbuild \
    --root build/dist \
    --identifier io.eventql.eventql \
    --version 0.3.0 \
    --ownership recommended \
    build/workspace/osxpkg/eventql.pkg

productbuild \
    --distribution build/scripts/package_osxpkg/distribution.xml \
    --resources build/workspace/osxpkg \
    --package-path build/workspace/osxpkg \
    --version 0.3.0 \
    build/package/eventql_v0.3.0_x86-64.pkg
