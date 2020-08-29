# text-search
Text-search is a GUI Program to find files and text within files for Linux. <br>
text-search is a single and small executable.

https://aldrik.itch.io/text-search

# Requirements

### Linux
- GCC
- libglu1-mesa-dev, libgl1-mesa-dev, libxrandr-dev (automatically installed with build/install script)
- ld

## Windows
- MinGW32
- Windres
- ld

# Build/Install
run __build-linux.sh -r__ or __build-win.bat -r__ for building and running a debug build<br />
run __release-linux.sh__ or __release-windows.bat as root to install to __/usr/local/bin/text-search__ or __C:\Users\\\<user>\\Desktop\\text-search.exe__ <br />

# Config
config.txt is stored at __~/.config/text-search/config.txt__ or __C:\Users\\\<user>\Local Settings\Application Data\text-search\config.txt__
