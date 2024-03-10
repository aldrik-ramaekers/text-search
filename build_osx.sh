shopt -s extglob

OUT_DIR="bin/debug"
FLAGS="-g3 -w"

if [[ $* == *-release* ]]; then
	OUT_DIR="bin/release"
	FLAGS="-O3"
fi

mkdir -p $OUT_DIR

# misc items are converted to header files, not embedded. (xxd -i LICENSE misc/osx/LICENSE.h)

g++ -m64 -std=c++17 $FLAGS -DUNICODE -o $OUT_DIR/text-search imgui/imgui*.cpp imgui/backends/imgui_impl_glfw.cpp src/widgets/*.cpp src/*.cpp imfiledialog/*.cpp src/osx/*.cpp -Iimgui -Iimgui/backends -Isrc -Isrc/osx -pthread -ldl -lglfw -lGL

if [[ $* == *-r* ]]; then
	./$OUT_DIR/text-search
fi