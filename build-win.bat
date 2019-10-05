@echo off

call C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat x64

mkdir C:\work\text_search\bin
pushd C:\work\text_search\bin
cl /Od /nologo /TC /Z7 /MP4 C:\work\text_search\src\project_base.c /Fe"C:\work\text_search\bin\text-search.exe" 
popd