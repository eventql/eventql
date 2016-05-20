#!/bin/bash
set -e

mkdir -p build/target/x86_64-apple-darwin
( \
  cd build/target/x86_64-apple-darwin && \
  ../../../configure --host=x86_64-apple-darwin \
        CC=clang \
        CXX=clang++ \
        CFLAGS="-mmacosx-version-min=10.6 -isysroot $OSXSDK" \
        CXXFLAGS="-mmacosx-version-min=10.6 -isysroot $OSXSDK" \
        LDFLAGS="-mmacosx-version-min=10.6 -isysroot $OSXSDK" && \
  make \
)
