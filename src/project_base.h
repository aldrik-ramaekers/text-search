/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
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

#include "asset_definitions.h"

#include "project-base/src/thread.h"
#include "project-base/src/array.h"
#include "project-base/src/memory.h"

#define STB_IMAGE_IMPLEMENTATION
#include "project-base/src/external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "project-base/src/external/stb_truetype.h"

#include "project-base/src/external/utf8.h"
#include "project-base/src/input.h"
#include "project-base/src/assets.h"
#include "project-base/src/memory_bucket.h"
#include "project-base/src/platform.h"
#include "project-base/src/render.h"
#include "project-base/src/camera.h"
#include "project-base/src/ui.h"
#include "project-base/src/string_utils.h"
#include "project-base/src/settings_config.h"
#include "project-base/src/localization.h"

#include "project-base/src/platform_shared.c"

#ifdef OS_LINUX
#define DEFAULT_DIRECTORY "/home/"
#define CONFIG_DIRECTORY "/.config/text-search"

#include "project-base/src/linux/thread.c"
#include "project-base/src/linux/platform.c"
#endif

#ifdef OS_WIN
#define DEFAULT_DIRECTORY "C:/"
#define CONFIG_DIRECTORY "\\text-search"

#include "project-base/src/windows/thread.c"
#include "project-base/src/windows/platform.c"
#endif

#include "project-base/src/input.c"
#include "project-base/src/array.c"
#include "project-base/src/assets.c"
#include "project-base/src/render.c"
#include "project-base/src/camera.c"
#include "project-base/src/ui.c"
#include "project-base/src/string_utils.c"
#include "project-base/src/settings_config.c"
#include "project-base/src/localization.c"
#include "project-base/src/memory_bucket.c"

#include "project-base/src/external/cJSON.h"
#include "project-base/src/external/cJSON.c"

#endif