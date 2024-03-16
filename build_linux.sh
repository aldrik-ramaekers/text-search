shopt -s extglob

OUT_DIR="bin/debug"
FLAGS="-g3 -Wall -DTS_DEBUG"
OUT_EXE="TextSearch"

if [[ $* == *-release* ]]; then
	OUT_DIR="bin/release"
	FLAGS="-O3 -DTS_RELEASE"
	OUT_EXE="TextSearch-x86_64"
fi

mkdir -p $OUT_DIR
g++ -m64 -std=c++17 $FLAGS -DUNICODE -o $OUT_DIR/$OUT_EXE imgui/imgui*.cpp imgui/backends/imgui_impl_glfw.cpp src/widgets/*.cpp src/*.cpp imfiledialog/*.cpp src/unix/*.cpp -Iimgui -Iimgui/backends -Isrc -Isrc/unix -pthread -ldl -lglfw -lGLU -lGL

if [[ $* == *-r* ]]; then
	./$OUT_DIR/$OUT_EXE
fi