call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
@set OUT_DIR=bin\\debug
@set OUT_EXE=text-search
@set INCLUDES=/I..\.. /I..\..\backends
@set SOURCES=imgui/imgui*.cpp src/*.cpp imfiledialog/*.cpp src/windows/*.cpp imgui/backends/imgui_impl_win32.cpp src/widgets/*.cpp
@set LIBS=opengl32.lib Advapi32.lib Shell32.lib Ole32.lib User32.lib Pathcch.lib bin/debug/icon.res
@set FLAGS=/DTS_DEBUG /DEBUG:FULL /Ob0 /MT /Oy- /Zi 
windres misc/icon.rc -O coff -o bin/debug/icon.res

if "%1"=="-a" (
	@set FLAGS=/analyze:external- /analyze:stacksize 40000
)

if "%1"=="-release" (
	@set OUT_DIR=bin\\release
	@set FLAGS=/GL /O2 /DTS_RELEASE
)

mkdir %OUT_DIR%
cl  /std:c++17 /nologo %FLAGS% /W3 /MD /EHsc /Isrc/windows /external:W0 /external:Iimgui /external:Iimgui/backends /Isrc /utf-8 %INCLUDES% /D UNICODE /D _UNICODE %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fd%OUT_DIR%/vc140.pdb /Fo%OUT_DIR%/ /link %LIBS%
if "%1"=="-r" call "bin/debug/text-search.exe"
if "%1"=="-d" call devenv "bin/debug/text-search.exe"
