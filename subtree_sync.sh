#!/bin/bash -e

git subtree pull --prefix app/metricd git@github.com:fnordcorp/metricd.git master
git subtree pull --prefix app/sensord git@github.com:fnordcorp/sensord.git master
git subtree pull --prefix app/tsdb git@github.com:fnordcorp/tsdb.git master
git subtree pull --prefix app/brokerd git@github.com:fnordcorp/brokerd.git master
git subtree pull --prefix app/chartsql git@github.com:fnordcorp/chartsql.git master
git subtree pull --prefix lib/libsstable git@github.com:fnordcorp/libsstable.git master
git subtree pull --prefix lib/libcstable git@github.com:fnordcorp/libcstable.git master
git subtree pull --prefix lib/libdproc git@github.com:fnordcorp/libdproc.git master
git subtree pull --prefix lib/libfnord git@github.com:fnordcorp/libfnord.git master

git subtree push --prefix app/sensord git@github.com:fnordcorp/sensord.git master
git subtree push --prefix app/metricd git@github.com:fnordcorp/metricd.git master
git subtree push --prefix app/brokerd git@github.com:fnordcorp/brokerd.git master
git subtree push --prefix app/tsdb git@github.com:fnordcorp/tsdb.git master
git subtree push --prefix app/chartsql git@github.com:fnordcorp/chartsql.git master
git subtree push --prefix lib/libsstable git@github.com:fnordcorp/libsstable.git master
git subtree push --prefix lib/libcstable git@github.com:fnordcorp/libcstable.git master
git subtree push --prefix lib/libdproc git@github.com:fnordcorp/libdproc.git master
git subtree push --prefix lib/libfnord git@github.com:fnordcorp/libfnord.git master

