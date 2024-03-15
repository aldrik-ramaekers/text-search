#pragma once

#include "array.h"
#include "memory_bucket.h"
#include "search.h"
#include "config.h"
#include "../utf8.h"

typedef struct t_ts_file_content
{
	size_t content_length;
	void *content;
	uint16_t file_error;
} ts_file_content;

typedef enum t_ts_file_open_error
{
	FILE_ERROR_NONE,
	FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS = 1,
	FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM = 2,
	FILE_ERROR_NO_ACCESS = 3,
	FILE_ERROR_NOT_FOUND = 4,
	FILE_ERROR_CONNECTION_ABORTED = 5,
	FILE_ERROR_CONNECTION_REFUSED = 6,
	FILE_ERROR_NETWORK_DOWN = 7,
	FILE_ERROR_REMOTE_IO_ERROR = 8,
	FILE_ERROR_STALE = 9, // NFS server file is removed/renamed
	FILE_ERROR_GENERIC = 10,
	FILE_ERROR_TOO_BIG = 11,
} ts_file_open_error;

typedef struct t_ts_dragdrop_data
{
	bool did_drop;
	utf8_int8_t path[MAX_INPUT_LENGTH];
	bool is_dragging_file;
} ts_dragdrop_data;


extern bool program_running;
extern ts_dragdrop_data dragdrop_data;

bool 			ts_platform_dir_exists(utf8_int8_t* dir);
ts_file_content ts_platform_read_file(char *path, const char *mode);
void 			ts_platform_list_files_block(ts_search_result* result, wchar_t* start_dir = NULL);
uint64_t 		ts_platform_get_time(uint64_t compare = 0); // if compare is not 0, return difference between timestamp and now, in milliseconds.
void			ts_platform_open_file_as(utf8_int8_t* str);
void			ts_platform_open_file_in_folder(utf8_int8_t* file);
void			ts_platform_set_window_title(utf8_int8_t* str);
