/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.

*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.

*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/* 
 *  Compile flags:
*  Linux: -lX11 -lGL -lGLU -lXrandr -lm -ldl
*
*/

#ifndef INCLUDE_PROJECT_BASE
#define INCLUDE_PROJECT_BASE

#ifdef _WIN32
#define OS_WIN
#include <windows.h>
#include <time.h>
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

#ifdef OS_LINUX
#define bool uint8_t
#endif
#ifdef OS_WIN
#define bool _Bool
#endif

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

#include "platform_shared.c"

#ifdef OS_LINUX
#include "linux/thread.c"
#include "linux/platform.c"
#endif

#ifdef OS_WIN
#include "windows/thread.c"
#include "windows/platform.c"
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