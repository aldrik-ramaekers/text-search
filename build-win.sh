#!/bin/bash

rm -rf bin
mkdir bin
cd src
x86_64-w64-mingw32-gcc -Wall -g -Wno-unused-label -Wno-unused-variable text_search.c -o ../bin/text-search.exe -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lole32 -lshlwapi

if [ $? -ne 0 ]; then
	cd ../
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
