#!/bin/bash

rm release/text-search_x64_linux.zip
rm release/text-search_x64_linux_trial.zip

cd src
gcc -Wall -O3 -m64 -Wno-unused-label -Wno-unused-variable text_search.c -o ../release/text-search_x64_linux -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

gcc -Wall -O3 -m64 -Wno-unused-label -DBUILD_TRIAL -Wno-unused-variable text_search.c -o ../release/text-search_x64_linux_trial -lX11 -lGL -lGLU -lXrandr -lm -lpthread -ldl

if [ $? -ne 0 ]; then
	cd ../
	exit 1
fi

cd ../

cp -r data/ release/
printf "SEARCH_DIRECTORY = \"/home/user/Projects/\"\nSEARCH_DIRECTORIES = \"1\"\nSEARCH_TEXT = \"*hello world*\"\nFILE_FILTER = \"*.txt,*.c\"\nMAX_THEAD_COUNT = \"20\"\nMAX_FILE_SIZE = \"200\"\nLOCALE = \"en\"\nWINDOW_WIDTH = \"800\"\nWINDOW_HEIGHT = \"600\"\nPARALLELIZE_SEARCH = \"1\"\n" > release/data/config.txt

cd release/

zip text-search_x64_linux.zip text-search_x64_linux -r data
zip text-search_x64_linux_trial.zip text-search_x64_linux_trial -r data

rm text-search_x64_linux
rm text-search_x64_linux_trial
rm -rf data/