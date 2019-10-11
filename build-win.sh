#!/bin/bash

if ! type "$windres" > /dev/null 2>/dev/null; then
	x86_64-w64-mingw32-windres misc/icon.rc -O coff -o misc/icon.res
else
	windres misc/icon.rc -O coff -o misc/icon.res
fi

rm -rf bin
mkdir bin
cd src
x86_64-w64-mingw32-gcc -Wall -g -Wno-unused-label -Wno-unused-variable text_search.c -o ../bin/text-search.exe ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lgdiplus -lole32 -lshlwapi

if [ $? -ne 0 ]; then
	cd ../
	$SHELL
	exit 1
fi

cd ../

cp -r data/ bin/

if [ "$1" == "-r" ]; then
	cd bin
   wine ./text-search.exe
	cd ..
fi

if [ "$1" == "-w" ]; then
	cd bin
   gdb -ex run ./text-search.exe
	cd ..
fi
