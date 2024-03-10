#include "definitions.h"

#ifdef __APPLE__
#include "../misc/osx/LICENSE.h"
unsigned char* _binary_LICENSE_start = LICENSE;
unsigned char* _binary_LICENSE_end = _binary_LICENSE_start + LICENSE_len;

#include "../misc/osx/imgui_LICENSE.h"
unsigned char* _binary_imgui_LICENSE_start = imgui_LICENSE;
unsigned char* _binary_imgui_LICENSE_end = imgui_LICENSE + imgui_LICENSE_len;

#include "../misc/osx/imfiledialog_LICENSE.h"
unsigned char* _binary_imfiledialog_LICENSE_start = imfiledialog_LICENSE;
unsigned char* _binary_imfiledialog_LICENSE_end = imfiledialog_LICENSE + imfiledialog_LICENSE_len;

#include "../misc/osx/logo_64.h"
unsigned char* _binary_misc_logo_64_png_start = misc_logo_64_png;
unsigned char* _binary_misc_logo_64_png_end = misc_logo_64_png + misc_logo_64_png_len;

#include "../misc/osx/search.h"
unsigned char* _binary_misc_search_png_start = misc_search_png;
unsigned char* _binary_misc_search_png_end = misc_search_png + misc_search_png_len;

#include "../misc/osx/folder.h"
unsigned char* _binary_misc_folder_png_start = misc_folder_png;
unsigned char* _binary_misc_folder_png_end = misc_folder_png + misc_folder_png_len;
#endif