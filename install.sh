#!/bin/bash

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

echo "Creating default config.."
cp COPYING /opt/textsearch/

# create default config
cp -r data/ /opt/textsearch/
rm -rf /opt/textsearch/data/export/
mkdir /opt/textsearch/data/export/
printf "SEARCH_DIRECTORY = \"/home/user/Projects/\"\nSEARCH_DIRECTORIES = \"1\"\nSEARCH_TEXT = \"*hello world*\"\nFILE_FILTER = \"*.txt,*.c\"\nMAX_THEAD_COUNT = \"20\"\nMAX_FILE_SIZE = \"200\"\nLOCALE = \"en\"\nWINDOW_WIDTH = \"800\"\nWINDOW_HEIGHT = \"600\"\nPARALLELIZE_SEARCH = \"1\"\n" > /opt/textsearch/data/config.txt

sudo chmod 775 -R /opt/textsearch/
sudo chmod 777 /opt/textsearch/data/config.txt
echo "Done creating default config"
echo "Done. Program is installed at \"/opt/textsearch/\", symlink is installed as \"/usr/local/bin/text-search\""

elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then

windres misc/icon.rc -O coff -o misc/icon.res

rm -rf "C:/Program Files (x86)/textsearch/"
mkdir "C:/Program Files (x86)/textsearch/"
cd src

x86_64-w64-mingw32-gcc -Wall -m64 -O3 -Wno-unused-label -Wno-unused-variable text_search.c -o "C:/Program Files (x86)/textsearch/text-search.exe" ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lgdiplus -lole32 -lshlwapi

cd ../
cp -r data/ "C:/Program Files (x86)/textsearch/"



elif [ "$(uname)" == "Darwin" ]; then
    echo "OSX Platform not supported"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ]; then
	echo "32bit Windows versions not supported"
fi