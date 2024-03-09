shopt -s extglob

OUT_DIR="bin/debug"
FLAGS="-g3 -Wall"

if [[ $* == *-release* ]]; then
	OUT_DIR="bin/release"
	FLAGS="-g3 -O3"
fi

mkdir -p $OUT_DIR

ld -r -b binary -o bin/debug/data.obj LICENSE misc/logo_64.png imgui/LICENSE imfiledialog/LICENSE misc/search.png misc/folder.png
g++ -m64 -std=c++17 $FLAGS -DUNICODE -o $OUT_DIR/text-search imgui/imgui*.cpp imgui/backends/imgui_impl_glfw.cpp src/*.cpp imfiledialog/*.cpp src/linux/*.cpp bin/debug/data.obj -Iimgui -Iimgui/backends -Isrc -Isrc/linux -pthread -ldl -lglfw -lGLU -lGL

if [[ $* == *-r* ]]; then
	./$OUT_DIR/text-search
fi