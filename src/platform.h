#ifndef INCLUDE_PLATFORM
#define INCLUDE_PLATFORM

#include "array.h"
#include "memory_bucket.h"
#include "search.h"
#include "../utf8.h"

typedef struct t_file_content
{
	int content_length;
	void *content;
	int file_error;
} file_content;

typedef enum t_file_open_error
{
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
} file_open_error;

file_content platform_read_file(char *path, const char *mode);
void platform_list_files_block(search_result* result, wchar_t* start_dir = nullptr);
void platform_list_files(search_result* result);

#endif