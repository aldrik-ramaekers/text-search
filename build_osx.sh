#!/bin/sh

shopt -s extglob

OUT_DIR="bin/debug"
FLAGS="-g3 -w -DTS_DEBUG"

if [[ $* == *-release* ]]; then
	OUT_DIR="bin/release"
	FLAGS="-O3 -DTS_RELEASE"
fi

mkdir -p $OUT_DIR

# misc items are converted to header files, not embedded. (xxd -i LICENSE misc/osx/LICENSE.h)

g++ -m64 -std=c++17 $FLAGS -DUNICODE -o $OUT_DIR/text-search imgui/imgui*.cpp imgui/backends/imgui_impl_glfw.cpp src/widgets/*.cpp src/*.cpp imfiledialog/*.cpp src/unix/*.cpp mem/mem.cpp -Iimgui -Imem -Iimgui/backends -Isrc -Isrc/unix -pthread -ldl -lglfw -lGL

if [[ $* == *-release* ]]; then
	cp $OUT_DIR/text-search misc/text-search.app/Contents/MacOS/text-search
	install_name_tool -change /usr/local/opt/glfw/lib/libglfw.3.dylib "@executable_path/../Library/libglfw.3.dylib" misc/text-search.app/Contents/MacOS/text-search
	install_name_tool -change /usr/local/opt/mesa/lib/libGL.1.dylib "@executable_path/../Library/libGL.1.dylib" misc/text-search.app/Contents/MacOS/text-search
	cp -R misc/text-search.app bin/release/text-search.app
	rm misc/text-search.app/Contents/MacOS/text-search
fi

if [[ $* == *-r* ]]; then
	./$OUT_DIR/text-search
fi