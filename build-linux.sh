#!/bin/bash

if [ $(dpkg-query -W -f='${Status}' libglu1-mesa-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
	if [ "$EUID" -ne 0 ]
	then
	  apt-get install libglu1-mesa-dev;
	else
	  echo "Missing dependency: libglu1-mesa-dev, install this package or run this script as root"
	fi
fi

if [ $(dpkg-query -W -f='${Status}' libgl1-mesa-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
	if [ "$EUID" -ne 0 ]
	then
	  apt-get install libgl1-mesa-dev;
	else
	  echo "Missing dependency: libgl1-mesa-dev, install this package or run this script as root"
	fi
fi

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
