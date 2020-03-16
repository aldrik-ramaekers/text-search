@echo off

windres misc/icon.rc -O coff -o misc/icon.res

DEL /S /Q bin
cd src

ld -r -b binary -o ../bin/data.o ../data/imgs/en.png ../data/imgs/error.png ../data/imgs/folder.png ../data/imgs/nl.png ../data/imgs/search.png ../data/imgs/logo_64.png ../data/fonts/mono.ttf ../data/translations/en-English.mo ../data/translations/nl-Dutch.mo

if "%1"=="-w" (SET defs=-DMODE_DEVELOPER -DMODE_GDBDEBUG) else (SET defs=-DMODE_DEVELOPER)

x86_64-w64-mingw32-gcc -m64 -Wall -g %defs% -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search.exe ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32 -lgdiplus -lole32 -lshlwapi 

REM libs used for licensing: -lwininet -liphlpapi

DEL /Q "../bin/data.o"

FOR %%A IN ("../bin/text-search.exe") DO set size=%%~zA
echo size = %size%

cd ../

if "%1"=="-r" start bin/text-search.exe
if "%1"=="-w" start gdb -ex run bin/text-search.exe