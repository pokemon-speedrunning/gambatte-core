#!/bin/sh
set -e
echo "cd libgambatte && scons -c shlib && scons shlib"
cd "$(dirname "$0")/../libgambatte"
scons -c shlib
scons shlib
../scripts/.copy_bizhawk_after_build.sh
