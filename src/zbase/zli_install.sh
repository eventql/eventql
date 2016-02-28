#!/bin/bash
set -e
bpath=$(cd -P -- "$(dirname -- "$0")" && pwd -P) && bpath=$bpath/$(basename -- "$0")
while [ -h $bpath ]; do
    bpath=$(cd $(dirname -- "$bpath") && cd $(dirname -- "$(readlink $bpath)") && pwd)/$(basename -- "$(readlink $bpath)");
done

bash -xc "sudo cp $(dirname -- "$bpath")/zli /usr/local/bin"
