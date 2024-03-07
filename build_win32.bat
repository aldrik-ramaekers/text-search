@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
set __VSCMD_ARG_no_logo=""
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
@set OUT_DIR=bin\\debug
@set OUT_EXE=text-search
@set INCLUDES=/I..\.. /I..\..\backends
@set SOURCES=imgui/imgui*.cpp src/*.cpp
@set LIBS=opengl32.lib Advapi32.lib Shell32.lib bin/debug/data.obj bin/debug/icon.res
windres misc/icon.rc -O coff -o bin/debug/icon.res
ld -r -b binary -o bin/debug/data.obj LICENSE misc/logo_64.png imgui/LICENSE imspinner/LICENSE misc/search.png misc/folder.png
mkdir %OUT_DIR%
cl /nologo /Zi /MD /EHsc /utf-8 %INCLUDES% /D UNICODE /D _UNICODE %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fd%OUT_DIR%/vc140.pdb /Fo%OUT_DIR%/ /link %LIBS%
if "%1"=="-r" call "bin/debug/text-search.exe"
if "%1"=="-d" call devenv "bin/debug/text-search.exe"
