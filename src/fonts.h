#pragma once

#include <wchar.h>

#define FOREACH_FONT_RANGE(FONT_RANGE) \
        FONT_RANGE(FONT_RANGE_ENGLISH)   \
        FONT_RANGE(FONT_RANGE_GREEK)  \
        FONT_RANGE(FONT_RANGE_KOREAN)   \
        FONT_RANGE(FONT_RANGE_JAPANESE)  \
		FONT_RANGE(FONT_RANGE_CHINESE_FULL)   \
        FONT_RANGE(FONT_RANGE_CHINESE_SIMPLE)  \
        FONT_RANGE(FONT_RANGE_CYRILLIC)   \
        FONT_RANGE(FONT_RANGE_THAI)  \
		FONT_RANGE(FONT_RANGE_VIETNAMESE)   \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum t_ts_font_range {
	FOREACH_FONT_RANGE(GENERATE_ENUM)
} ts_font_range;

static const char *FONT_RANGE_STRING[] = {
    FOREACH_FONT_RANGE(GENERATE_STRING)
};

void 			ts_load_fonts(float size, ts_font_range locale);
ts_font_range 	ts_locale_to_range(wchar_t* locale);