shopt -s extglob

OUT_DIR="bin/debug"

mkdir -p "bin/debug"

ld -r -b binary -o bin/debug/data.obj LICENSE misc/logo_64.png imgui/LICENSE imfiledialog/LICENSE misc/search.png misc/folder.png
g++ -m64 -std=c++17 -Wall -DUNICODE -o $OUT_DIR/text-search imgui/imgui*.cpp imgui/backends/imgui_impl_glfw.cpp src/*.cpp imfiledialog/*.cpp src/linux/*.cpp bin/debug/data.obj -Iimgui -Iimgui/backends -Isrc -Isrc/linux -pthread -ldl -lglfw -lGLU -lGL


./bin/debug/text-search