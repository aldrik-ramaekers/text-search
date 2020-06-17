@echo off

windres misc/icon.rc -O coff -o misc/icon.res

DEL /S /Q bin
cd src

ld -r -b binary -o ../bin/data.o ../data/imgs/en.bmp ../data/imgs/error.bmp ../data/imgs/folder.bmp ../data/imgs/nl.bmp ../data/imgs/search.bmp ../data/imgs/logo_64.bmp ../data/fonts/mono.ttf ../data/translations/en-English.mo ../data/translations/nl-Dutch.mo

SET defs=-DMODE_DEVELOPER
if "%1"=="-w" (SET defs=-DMODE_DEVELOPER -DMODE_GDBDEBUG)
if "%2"=="-t" (SET defs=-DMODE_DEVELOPER -DMODE_TEST)
if "%1"=="-ti" (SET defs=-DMODE_TIMESTARTUP -DMODE_DEVELOPER)

x86_64-w64-mingw32-gcc -m64 -Wall -g %defs% -Wno-unused-label -Wno-unused-variable text_search.c ../bin/data.o -o ../bin/text-search.exe ../misc/icon.res -lopengl32 -lkernel32 -lglu32 -lgdi32 -lcomdlg32

DEL /Q "../bin/data.o"

FOR %%A IN ("../bin/text-search.exe") DO set size=%%~zA
echo size = %size%

cd ../

if "%1"=="-r" start bin/text-search.exe
if "%1"=="-w" start gdb -ex run bin/text-search.exe

if "%1"=="-ti" (PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& ((Measure-Command { for($i = 0; $i -lt 10; $i++) { .\bin\text-search.exe } }).TotalMilliseconds / 10)")