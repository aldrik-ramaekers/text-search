#!/bin/bash

rm -rf release/
mkdir release

cd src
gcc -Wall -O3 -m64 -Wno-unused-label -Wno-unused-variable text_search.c -o ../release/text-search_x64_linux -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

if [ $? -ne 0 ]; then
	cd ../
	exit 1
fi

cd ../

# delete git files and add util files
cp -r src/ release/
rm -rf release/src/.git
rm release/src/.gitignore
cp build.sh release/
cp COPYING release/
cp build-release-linux.sh release/

# create default config
cp -r data/ release/
rm -rf release/data/export/
mkdir release/data/export/
printf "SEARCH_DIRECTORY = \"/home/user/Projects/\"\nSEARCH_DIRECTORIES = \"1\"\nSEARCH_TEXT = \"*hello world*\"\nFILE_FILTER = \"*.txt,*.c\"\nMAX_THEAD_COUNT = \"20\"\nMAX_FILE_SIZE = \"200\"\nLOCALE = \"en\"\nWINDOW_WIDTH = \"800\"\nWINDOW_HEIGHT = \"600\"\nPARALLELIZE_SEARCH = \"1\"\n" > release/data/config.txt

cd release/

tar -cvzf text-search_x64_linux.tar.gz text-search_x64_linux data
tar -cvzf text-search_x64_linux_source.tar.gz build.sh build-release-linux.sh data src

rm text-search_x64_linux
rm -rf data/
rm -rf src/
rm build.sh
rm build-release-linux.sh
rm COPYING