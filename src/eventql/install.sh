#!/bin/bash
PREFIX=/usr/local

set -e
bpath=$(cd -P -- "$(dirname -- "$0")" && pwd -P) && bpath=$bpath/$(basename -- "$0")
while [ -h $bpath ]; do
    bpath=$(cd $(dirname -- "$bpath") && cd $(dirname -- "$(readlink $bpath)") && pwd)/$(basename -- "$(readlink $bpath)");
done

cd $(dirname -- "$bpath")
cp -R bin $PREFIX
