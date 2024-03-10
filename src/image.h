#ifndef INCLUDE_IMAGE
#define INCLUDE_IMAGE

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>

typedef struct t_ts_image {
	GLuint id;
	int width;
	int height;
} ts_image;
#elif defined(__linux__) || defined(__APPLE__)

//#include <GL/glut.h>
#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"
#include "imgui_impl_opengl3.h"

typedef struct t_ts_image {
	GLuint id;
	int width;
	int height;
} ts_image;
#endif

extern ts_image img_logo;
extern ts_image img_search;
extern ts_image img_folder;

void ts_load_images();

#endif