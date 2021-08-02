#!/bin/sh
set -e
echo "cd libgambatte && scons shlib"
cd "$(dirname "$0")/../libgambatte"
scons shlib
../scripts/.copy_bizhawk_after_build.sh
