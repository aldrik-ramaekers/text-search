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

if [ $(dpkg-query -W -f='${Status}' libxrandr-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
	if [ "$EUID" -ne 0 ]
	then
	  echo "Missing dependency: libxrandr-dev, install this package or run this script as root"
	else
	  apt-get install libxrandr-dev;
	fi
fi

mkdir -p release
rm -rf release/linux
mkdir release/linux

cd src

ld -r -b binary -o ../bin/data.o ../data/imgs/en.bmp ../data/imgs/error.bmp ../data/imgs/folder.bmp ../data/imgs/nl.bmp ../data/imgs/search.bmp ../data/imgs/logo_64.bmp ../data/fonts/mono.ttf ../data/translations/en-English.mo ../data/translations/nl-Dutch.mo

gcc -Wall -O3 -m64 -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

rm -f ../bin/data.o
cp --remove-destination ../bin/text-search ../release/linux/text-search

cd ..
