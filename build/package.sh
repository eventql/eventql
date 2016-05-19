#!/bin/bash
set -e

PKG_NAME=eventql
PKG_DIR=build/package
DIST_DIR=build/dist
TMP_DIR=build/workspace/tarball
TARGET_LBL=$1

if [[ "$TARGET_LBL" == "host" ]]; then
  TARGET_LBL="$(uname | tr '[:upper:]' '[:lower:]')_$(uname -m)"
fi

if [[ -z "$TARGET_LBL" ]]; then
  echo "usage: $0 <TARGET_LBL>";
  exit 1
fi

mkdir -p $PKG_DIR $TMP_DIR/$PKG_NAME
cp -R $DIST_DIR/* $TMP_DIR
tar cz -C $TMP_DIR $PKG_NAME > $PKG_DIR/$PKG_NAME-$TARGET_LBL.tgz
