#pragma once

#include "search.h"
#include "platform.h"

typedef enum ts_export_result {
	EXPORT_NONE,
	EXPORT_NO_RESULT,
	EXPORT_SEARCH_ACTIVE,
	EXPORT_SAVE_PENDING,
} export_result;

bool 			ts_str_has_extension(const utf8_int8_t *str, const utf8_int8_t *suffix);
export_result 	ts_export_result(ts_search_result* result, const utf8_int8_t* path);