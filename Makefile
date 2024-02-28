MAKEFLAGS += -s
MAKEFLAGS += --always-make

main_file = src/text-search.c
output_file = text-search

ifeq ($(OS), Windows_NT)
	permissions = 
	libs = 
else
	permissions = sudo
	libs = -lX11 -lm -ldl
endif

all:
	make build

empty:
	@$(NOECHO) $(NOOP)

# Build (Windows + Linux)
build:
	windres misc/icon.rc -O coff -o misc/icon.res
	$(permissions) rm -rf "bin"
	mkdir "bin"
	ld -r -b binary -o bin/data.o data/imgs/en.bmp data/imgs/error.bmp data/imgs/folder.bmp data/imgs/nl.bmp data/imgs/search.bmp data/imgs/logo_64.bmp data/fonts/mono.ttf data/translations/en-English.mo data/translations/nl-Dutch.mo
	x86_64-w64-mingw32-gcc -m64 -Wall -g -Wno-unused-label -Wno-unused-variable src/text_search.c bin/data.o -o bin/text-search.exe misc/icon.res -lprojectbase
	rm "bin/data.o"

run:
	make build
	.\bin\text-search.exe