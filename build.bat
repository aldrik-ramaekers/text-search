@echo off

set __VSCMD_ARG_no_logo=""

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

DEL /S /Q bin >nul

mkdir bin >nul 2>nul

rc misc/icon.rc

cd bin
cl -DMODE_DEVELOPER /std:c11 -Zi /nologo ..\src\text_search.c ..\misc\icon.res user32.lib gdi32.lib winmm.lib shlwapi.lib opengl32.lib kernel32.lib glu32.lib comdlg32.lib dbghelp.lib advapi32.lib shell32.lib
cd ..
