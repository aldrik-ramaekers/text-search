#pragma once

#include <wchar.h>

typedef enum t_ts_font_range {
	FONT_RANGE_ENGLISH,
	FONT_RANGE_GREEK,
	FONT_RANGE_KOREAN,
	FONT_RANGE_JAPANESE,
	FONT_RANGE_CHINESE_FULL,
	FONT_RANGE_CHINESE_SIMPLE,
	FONT_RANGE_CYRILLIC,
	FONT_RANGE_THAI,
	FONT_RANGE_VIETNAMESE,
} ts_font_range;

void 			ts_load_fonts(float size, ts_font_range locale);
ts_font_range 	ts_locale_to_range(wchar_t* locale);