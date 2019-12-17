#!/bin/bash

if [ $(dpkg-query -W -f='${Status}' libglu1-mesa-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
	if [ "$EUID" -ne 0 ]
	then
	  echo "Missing dependency: libglu1-mesa-dev, install this package or run this script as root"
	else
	  apt-get install libglu1-mesa-dev;
	fi
fi

if [ $(dpkg-query -W -f='${Status}' libgl1-mesa-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
	if [ "$EUID" -ne 0 ]
	then
	  echo "Missing dependency: libgl1-mesa-dev, install this package or run this script as root"
	else
	  apt-get install libgl1-mesa-dev;
	fi
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

gcc -Wall -g -m64 -DMODE_DEVELOPER -Wno-unused-label -rdynamic -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

rm -f ../bin/data.o

if [ $? -ne 0 ]; then
	cd ../
	exit 1
fi

cd ../

cp data/config.txt bin/config.txt

if [ "$1" == "-r" ]; then
	cd bin
   ./text-search
	cd ..
fi
