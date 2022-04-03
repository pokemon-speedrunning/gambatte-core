#!/bin/sh
set -e
echo "cd libgambatte && scons shlib $1"
cd "$(dirname "$0")/../libgambatte"
scons shlib $1
../scripts/.copy_bizhawk_after_build.sh
