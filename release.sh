rm -rf release
mkdir release
rm -rf release/linux
mkdir release/linux
rm -rf release/windows
mkdir release/windows
rm -rf bin
mkdir bin

###################################
# linux
###################################

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
cp --remove-destination ../bin/text-search ../release/linux/text-search

cd ..


###################################
# windows
###################################

cd src 

x86_64-w64-mingw32-ld -r -b binary -o ../bin/data.o \
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
cp --remove-destination ../bin/text-search.exe ../release/windows/text-search.exe
