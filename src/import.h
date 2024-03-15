#pragma once

#include "search.h"

typedef enum t_import_result {
	IMPORT_NONE,
	IMPORT_INVALID_DATA,
	IMPORT_INVALID_VERSION,
	IMPORT_FILE_ERROR,
} import_result;

extern import_result last_import_result;

ts_search_result* 	ts_import_result(const utf8_int8_t* path);
void				ts_create_import_popup(int window_w, int window_h);