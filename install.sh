#!/bin/bash

########################################################################
########################################################################
if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then

if [ "$EUID" -ne 0 ]
  then echo "Please run this script as root."
  exit
fi

echo "Checking if dependencies are installed.."
if [ $(dpkg-query -W -f='${Status}' libglu1-mesa-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
  apt-get install libglu1-mesa-dev;
fi


if [ $(dpkg-query -W -f='${Status}' libgl1-mesa-dev 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
  apt-get install libgl1-mesa-dev;
fi
echo "Dependencies are installed"

echo "Compiling program.."
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

gcc -Wall -O3 -m64 -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

rm -f ../bin/data.o

echo "Done compiling program"

cp --remove-destination ../bin/text-search /usr/local/bin/text-search

cd ../

########################################################################
########################################################################
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then

windres misc/icon.rc -O coff -o misc/icon.res

echo "Compiling program.."
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

x86_64-w64-mingw32-gcc -Wall -m64 -O3 -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search.exe ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lgdiplus -lole32 -lshlwapi

rm -f ../bin/data.o

echo "Done compiling program, text-search.exe is located in 'C:\Manually installed'"

cp --remove-destination "../bin/text-search.exe" "C:\Manually installed programs\text-search.exe"

cd ../

########################################################################
########################################################################
elif [ "$(uname)" == "Darwin" ]; then
    echo "OSX Platform not supported"
########################################################################
########################################################################
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ]; then
	echo "32bit Windows versions not supported"
fi