# text-search
Text-search is a GUI Program to find files and text within files for x64 Windows and Linux. <br>
text-search is a single and small executable.

https://aldrik.itch.io/text-search
<p float="left">
<img src="https://img.itch.zone/aW1hZ2UvNTQxMDA3LzI4NDM3NDkucG5n/original/JWVbEx.png" width="400">
<img src="https://img.itch.zone/aW1hZ2UvNTQxMDA3LzI4NDM3NDcucG5n/original/8pR%2BhY.png" width="300">
</p>

# Requirements

### Linux
- GCC
- libglu1-mesa-dev, libgl1-mesa-dev, libxrandr-dev
- ld

## Windows
- MinGW32
- Windres
- ld

# Build/Install
run __build-linux.sh -r__ or __build-win.bat -r__ for building and running a debug build<br />
run __release-linux.sh__ or __release-windows.bat__ as to building release build<br />

# Deps
All the code required to build is inside the repository.
External libraries used:
- utf8.h https://github.com/sheredom/utf8.h
- stb_truetype.h
- stb_image.h
- cJSON

# Config
config.txt is stored at __~/.config/text-search/config.txt__ or __C:\Users\\\<user>\Local Settings\Application Data\text-search\config.txt__
