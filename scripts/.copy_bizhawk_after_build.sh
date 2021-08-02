#!/bin/sh
cd "$(dirname "$0")/../libgambatte"
if [ -e "../../../BizHawk.sln" ]; then
	echo "copying output for BizHawk..."
	cp -v -t ../../../Assets/dll libgambatte.dll libgambatte.so
	if [ -e "../../../output/dll" ]; then
		cp -v -t ../../../output/dll libgambatte.dll libgambatte.so
	fi
fi
