#!/bin/sh

CC=$1 CXX=$2 RANLIB=$3 AR=$4 ./configure || exit 1
CC=$1 CXX=$2 RANLIB=$3 AR=$4 make || exit 1
