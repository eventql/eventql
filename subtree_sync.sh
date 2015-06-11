#!/bin/bash -e

git subtree pull --prefix 3rdparty/tsdb git@github.com:fnordcorp/tsdb.git master
git subtree push --prefix 3rdparty/tsdb git@github.com:fnordcorp/tsdb.git master

git subtree pull --prefix 3rdparty/sensord git@github.com:fnordcorp/sensord.git master
git subtree push --prefix 3rdparty/sensord git@github.com:fnordcorp/sensord.git master

git subtree pull --prefix 3rdparty/brokerd git@github.com:fnordcorp/brokerd.git master
git subtree push --prefix 3rdparty/brokerd git@github.com:fnordcorp/brokerd.git master

git subtree pull --prefix 3rdparty/libfnord git@github.com:fnordcorp/libfnord.git master
git subtree push --prefix 3rdparty/libfnord git@github.com:fnordcorp/libfnord.git master
