#!/bin/bash

mkdir -p release
rm -rf release/linux
mkdir release/linux

cd src

ld -r -b binary -o ../bin/data.o \
../data/imgs/en.png \
../data/imgs/error.png \
../data/imgs/folder.png \
../data/imgs/nl.png \
../data/imgs/search.png \
../data/imgs/logo_64.png \
../data/fonts/mono.ttf \
../data/translations/en-English.mo \
../data/translations/nl-Dutch.mo \

gcc -Wall -O3 -m64 -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl -lcurl

rm -f ../bin/data.o
cp --remove-destination ../bin/text-search ../release/linux/text-search

cd ..
