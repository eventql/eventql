#!/bin/bash
set -e

mkdir -p build/target/i386-linux-gnu
(cd build/target/i386-linux-gnu && ../../../configure --host=i386-linux-gnu && make)

