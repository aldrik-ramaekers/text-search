#!/bin/bash

rm -rf release
mkdir release
mkdir release/linux
cd src
gcc -Wall -O3 -m64 -Wno-unused-label -rdynamic -Wno-unused-variable text_search.c -o ../release/linux/text-search_x64_linux -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

gcc -Wall -O3 -m64 -Wno-unused-label -DBUILD_TRIAL -rdynamic -Wno-unused-variable text_search.c -o ../release/linux/text-search_x64_linux_trial -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

if [ $? -ne 0 ]; then
	cd ../
	exit 1
fi

cd ../

cp -r data/ release/linux/
