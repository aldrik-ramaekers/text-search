#!/bin/sh

shopt -s extglob

OUT_DIR="bin/debug"
FLAGS="-g3 -w -DTS_DEBUG"
OUT_EXE="TextSearch"

if [[ $* == *-release* ]]; then
	OUT_DIR="bin/release"
	FLAGS="-O3 -DTS_RELEASE"
	OUT_EXE="TextSearch-x86_64"
fi

mkdir -p $OUT_DIR

# misc items are converted to header files, not embedded. (xxd -i LICENSE misc/osx/LICENSE.h)

g++ -m64 -std=c++17 $FLAGS -DUNICODE -o $OUT_DIR/$OUT_EXE imgui/imgui*.cpp imgui/backends/imgui_impl_glfw.cpp src/widgets/*.cpp src/*.cpp imfiledialog/*.cpp src/unix/*.cpp -Iimgui -Iimgui/backends -Isrc -Isrc/unix -pthread -ldl -lglfw -lGL

if [[ $* == *-release* ]]; then
	cp $OUT_DIR/$OUT_EXE misc/TextSearch-x86_64.app/Contents/MacOS/text-search
	install_name_tool -change /usr/local/opt/glfw/lib/libglfw.3.dylib "@executable_path/../Library/libglfw.3.dylib" misc/TextSearch-x86_64.app/Contents/MacOS/text-search
	install_name_tool -change /usr/local/opt/mesa/lib/libGL.1.dylib "@executable_path/../Library/libGL.1.dylib" misc/TextSearch-x86_64.app/Contents/MacOS/text-search
	cp -R misc/TextSearch-x86_64.app bin/release/TextSearch-x86_64.app
	rm misc/TextSearch-x86_64.app/Contents/MacOS/text-search
fi

if [[ $* == *-r* ]]; then
	./$OUT_DIR/$OUT_EXE
fi