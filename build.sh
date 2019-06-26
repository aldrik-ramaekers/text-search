#!/bin/bash

rm -rf bin
mkdir bin
cd src
gcc -Wall -g -m64 -DMODE_DEVELOPER -Wno-unused-label -rdynamic -Wno-unused-variable text_search.c -o ../bin/text-search -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

if [ $? -ne 0 ]; then
	cd ../
	exit 1
fi

cd ../

cp -r data/ bin/

if [ "$1" == "-r" ]; then
	cd bin
   ./text-search
	cd ..
fi
