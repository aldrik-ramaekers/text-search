#ifndef INCLUDE_IMAGE
#define INCLUDE_IMAGE

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

extern ts_image img_logo;
extern ts_image img_search;
extern ts_image img_folder;

void ts_load_images();

#endif