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
echo "dependencies are installed"

echo "Removing previous installation.."
rm -rf /opt/textsearch/
mkdir /opt/textsearch/

echo "Compiling program.."
cd src
gcc -Wall -O3 -m64 -Wno-unused-label -Wno-unused-variable text_search.c -o /opt/textsearch/text-search -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl
echo "Done compiling program"

ln -sf /opt/textsearch/text-search /usr/local/bin/text-search

if [ $? -ne 0 ]; then
	cd ../
	exit 1
fi

cd ../

echo "Copying data.."
cp COPYING /opt/textsearch/

cp -r data/ /opt/textsearch/
rm -rf /opt/textsearch/data/export/
mkdir /opt/textsearch/data/export/

sudo chmod 775 -R /opt/textsearch/
rm /opt/textsearch/data/config.txt
printf " " > /opt/textsearch/data/config.txt
sudo chmod 777 /opt/textsearch/data/config.txt

echo "Done copying data"
echo "Done. Program is installed at \"/opt/textsearch/\", symlink is installed as \"/usr/local/bin/text-search\""

########################################################################
########################################################################
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then

windres misc/icon.rc -O coff -o misc/icon.res

echo "Removing previous installation.."
rm -rf "C:/Program Files (x86)/textsearch/"
mkdir "C:/Program Files (x86)/textsearch/"
cd src

echo "Compiling program.."
x86_64-w64-mingw32-gcc -Wall -m64 -O3 -Wno-unused-label -Wno-unused-variable text_search.c -o "C:/Program Files (x86)/textsearch/text-search.exe" ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lgdiplus -lole32 -lshlwapi
echo "Done compiling program"

echo "Copying data.."
cd ../
cp -r data/ "C:/Program Files (x86)/textsearch/"

rm "C:/Program Files (x86)/textsearch/data/config.txt"

echo "Done copying data"
echo "Done. Program is installed at \"C:\\Program Files (x86)\\\""

########################################################################
########################################################################
elif [ "$(uname)" == "Darwin" ]; then
    echo "OSX Platform not supported"
########################################################################
########################################################################
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ]; then
	echo "32bit Windows versions not supported"
fi