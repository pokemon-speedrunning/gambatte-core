#!/bin/sh
cd "$(dirname "$0")/../libgambatte"
if [ -e "../../../BizHawk.sln" ]; then
	rm -f ../../../Assets/dll/libgambatte.*
	cp -v -t ../../../Assets/dll libgambatte.dll libgambatte.so
	if [ -e "../../../output/dll" ]; then
		rm -f ../../../output/dll/libgambatte.*
		cp -v -t ../../../output/dll libgambatte.*
	fi
fi
