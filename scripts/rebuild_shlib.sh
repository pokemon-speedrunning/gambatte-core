#!/bin/sh

echo "cd libgambatte && scons shlib"
(cd libgambatte && scons -c shlib && scons shlib)

if [ -d "../../Assets/" ]
then
echo "cp libgambatte/libgambatte.dll ../../Assets/dll/libgambatte.dll"
(cp libgambatte/libgambatte.dll ../../Assets/dll/libgambatte.dll)
echo "cp libgambatte/libgambatte.so ../../Assets/dll/libgambatte.dll.so"
(cp libgambatte/libgambatte.so ../../Assets/dll/libgambatte.dll.so)
fi

if [ -d "../../output/" ]
then
echo "cp libgambatte/libgambatte.dll ../../output/dll/dll/libgambatte.dll"
(cp libgambatte/libgambatte.dll ../../output/dll/dll/libgambatte.dll)
echo "cp libgambatte/libgambatte.so ../../output/dll/dll/libgambatte.dll.so"
(cp libgambatte/libgambatte.so ../../output/dll/dll/libgambatte.dll.so)
fi
