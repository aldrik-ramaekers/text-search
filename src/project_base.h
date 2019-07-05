/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This file is part of Text-search.
*
    *  Text-search is free software: you can redistribute it and/or modify
    *  it under the terms of the GNU General Public License as published by
    *  the Free Software Foundation, either version 3 of the License, or
    *  (at your option) any later version.
	
    *  Text-search is distributed in the hope that it will be useful,
    *  but WITHOUT ANY WARRANTY; without even the implied warranty of
    *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    *  GNU General Public License for more details.
	
    *  You should have received a copy of the GNU General Public License
    *  along with Text-search.  If not, see <https://www.gnu.org/licenses/>.
*/

/* 
*  Include this file in your project and add follow below instructions
*  
*  Optional definitions:
*  MODE_DEVELOPER                            - show info menu, including: profiling, system 
*                                              info
*
*  ASSET_IMAGE_COUNT                         - define the maximum amount of images that will be 
*                                              used at a given time
*  ASSET_FONT_COUNT                          - define the maximum amount of fonts that will be 
*                                              used at a given time
*  ASSET_SAMPLE_COUNT                        - define the maximum amount of samples that will 
*                                              be used at a given time
*  ASSET_QUEUE_COUNT                         - define the maximum queue size at a given time
*
*  WATCH_WINDOW_WIDTH                        - width if a watch window within the info menu.
*  TOOLTIP_BACKGROUND_COLOR                  - background color of the tooltip shown 
 *                                              when hovering over an item within the info menu.
*  TOOLTIP_FOREGROUND_COLOR                  - foreground color of the tooltip shown
 *                                              when hovering over an item within the info menu.
*  WATCH_WINDOW_ENTRY_HOVER_BACKGROUMD_COLOR - background color of the item when
 *                                              hovering over an item within the info menu.
*  WATCH_WINDOW_BACKGROUND_COLOR             - background color of watch window and info menu *                              
*                                              buttons.
*  WATCH_WINDOW_HOVER_BACKGROUND_COLOR       - background color of info menu button on hover.
*  WATCH_WINDOW_BORDER_COLOR                 - border color of info menu buttons.
*  WATCH_WINDOW_SCROLL_SPEED                 - scroll speed of watch window
*  CONSOLE_MESSAGE_COLOR                     - console message color for normal messages
*  CONSOLE_ERROR_COLOR                       - console message color for error messages
*
*
 *  Compile flags:
*  Linux: -lX11 -lGL -lGLU -lXrandr -lm -ldl
*  Windows:
*
*/

#ifndef INCLUDE_PROJECT_BASE
#define INCLUDE_PROJECT_BASE

int main_loop();

#ifdef _WIN32
#define OS_WINDOWS
#error  platform not supported
#endif
#ifdef __linux__
#define OS_LINUX
#include <sys/times.h>
#include <sys/vtimes.h>
#endif
#ifdef __APPLE__
#define OS_OSX
#error platform not supported
#endif

#include "stdint.h"
#include "string.h"
#include "assert.h"

#include <GL/gl.h>
#ifdef OS_LINUX
#include <GL/glx.h>
#endif
#include <GL/glu.h>
#include <GL/glext.h>

#define s8 int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define float32 float
#define float64 double

#define bool uint8_t
#define true 1
#define false 0

#include "thread.h"
#include "array.h"
#include "memory.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#include "input.h"
#include "assets.h"
#include "memory_bucket.h"
#include "platform.h"
#include "render.h"
#include "camera.h"
#include "ui.h"
#include "string_utils.h"
#include "settings_config.h"
#include "localization.h"
#include "text_buffer.h"

#ifdef OS_LINUX
#include "linux/thread.c"
#include "linux/platform.c"
#endif

#include "input.c"
#include "array.c"
#include "assets.c"
#include "render.c"
#include "camera.c"
#include "ui.c"
#include "string_utils.c"
#include "settings_config.c"
#include "localization.c"
#include "text_buffer.c"
#include "memory_bucket.c"

#ifdef MODE_DEVELOPER
#include "memory.c"
#endif

#endif