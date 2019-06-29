#!/bin/bash

rm text-search_x64_win.zip
rm text-search_x64_win_trial.zip

cd src
x86_64-w64-mingw32-gcc -Wall -O3 -m64 -Wno-unused-label -Wno-unused-variable text_search.c -o ../release/text-search_x64_win.exe -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lole32 -lshlwapi

x86_64-w64-mingw32-gcc -Wall -O3 -m64 -Wno-unused-label -DBUILD_TRIAL -Wno-unused-variable text_search.c -o ../release/text-search_x64_win_trial.exe -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lole32 -lshlwapi

if [ $? -ne 0 ]; then
	cd ../
	exit 1
fi

cd ../

cp -r data/ release/

cd release/

zip text-search_x64_win.zip text-search_x64_win.exe -r data
zip text-search_x64_win_trial.zip text-search_x64_win_trial.exe -r data

rm text-search_x64_win.exe
rm text-search_x64_win_trial.exe
rm -rf data/