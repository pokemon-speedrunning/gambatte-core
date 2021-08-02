#!/bin/sh

echo "cd libgambatte && scons -c ."
(cd libgambatte && scons -c .)

echo "cd test && scons -c"
(cd test && scons -c)

echo "rm -f *gambatte*/config.log"
rm -f *gambatte*/config.log

echo "rm -rf *gambatte*/.scon*"
rm -rf *gambatte*/.scon*

echo "rm -f test/config.log"
rm -f test/config.log

echo "rm -rf test/.scon*"
rm -rf test/.scon*
