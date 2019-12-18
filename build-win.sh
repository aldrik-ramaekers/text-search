#!/bin/bash

if ! type "$windres" > /dev/null 2>/dev/null; then
	x86_64-w64-mingw32-windres misc/icon.rc -O coff -o misc/icon.res
else
	windres misc/icon.rc -O coff -o misc/icon.res
fi

rm -rf bin
mkdir bin
cd src

ld -r -b binary -o ../bin/data.o \
../data/imgs/en.png \
../data/imgs/error.png \
../data/imgs/folder.png \
../data/imgs/nl.png \
../data/imgs/search.png \
../data/imgs/logo_32.png \
../data/imgs/logo_512.png \
../data/fonts/mono.ttf \
../data/translations/en-English.mo \
../data/translations/nl-Dutch.mo \

x86_64-w64-mingw32-gcc -m64 -Wall -g -DMODE_DEVELOPER -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search.exe ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lgdiplus -lole32 -lshlwapi

rm -f ../bin/data.o

if [ $? -ne 0 ]; then
	cd ../
	$SHELL
	exit 1
fi

cd ../

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
