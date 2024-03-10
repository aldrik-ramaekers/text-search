shopt -s extglob

OUT_DIR="bin/debug"
FLAGS="-g3 -Wall"

if [[ $* == *-release* ]]; then
	OUT_DIR="bin/release"
	FLAGS="-O3"
fi

mkdir -p $OUT_DIR
g++ -m64 -std=c++17 $FLAGS -DUNICODE -o $OUT_DIR/text-search imgui/imgui*.cpp imgui/backends/imgui_impl_glfw.cpp src/widgets/*.cpp src/*.cpp imfiledialog/*.cpp src/unix/*.cpp -Iimgui -Iimgui/backends -Isrc -Isrc/unix -pthread -ldl -lglfw -lGLU -lGL

if [[ $* == *-r* ]]; then
	./$OUT_DIR/text-search
fi