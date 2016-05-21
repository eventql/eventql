#!/bin/bash
set -e

mkdir -p build/target/x86_64-apple-darwin
mkdir -p build/target/i386-apple-darwin

( \
  cd build/target/x86_64-apple-darwin && \
  ../../../configure \
      --host=x86_64-apple-darwin10 \
      CC=o64-clang \
      CXX=o64-clang++ \
      CFLAGS="-mmacosx-version-min=10.6" \
      CXXFLAGS="-mmacosx-version-min=10.6" \
      LDFLAGS="-mmacosx-version-min=10.6" && \
  make
)
