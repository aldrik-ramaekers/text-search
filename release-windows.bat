windres misc/icon.rc -O coff -o misc/icon.res

DEL /S /Q "bin"
if not exist "release" mkdir "release"
if not exist "release" mkdir "release/windows"
DEL /S /Q "release/windows"

cd src 

ld -r -b binary -o ../bin/data.o ../data/imgs/en.png ../data/imgs/error.png ../data/imgs/folder.png ../data/imgs/nl.png ../data/imgs/search.png ../data/imgs/logo_64.png ../data/fonts/mono.ttf ../data/translations/en-English.mo ../data/translations/nl-Dutch.mo

x86_64-w64-mingw32-gcc -Wall -m64 -O3 -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search.exe ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lgdiplus -lole32 -lshlwapi -lwininet

DEL /Q "../bin/data.o"

move /y "../bin/text-search.exe" "../release/windows"

cd ..
