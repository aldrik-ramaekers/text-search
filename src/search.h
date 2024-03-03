#ifndef INCLUDE_SEARCH
#define INCLUDE_SEARCH

#define MAX_INPUT_LENGTH 4096
#define MAX_ERROR_MESSAGE_LENGTH (MAX_INPUT_LENGTH)
#define FILE_RESERVE_COUNT 1000
#define ERROR_RESERVE_COUNT 100

#include "array.h"
#include "memory_bucket.h"
#include "../utf8.h"

typedef struct t_ts_found_file
{
	utf8_int8_t *path;
	int match_count;
} ts_found_file;

typedef struct t_ts_search_result
{
	// data
	ts_array files;
	ts_array matches;
	ts_array filters;
	int match_count;
	int file_count;
	ts_memory_bucket memory;
	struct t_ts_search_result* prev_result;

	// thread syncing
	ts_mutex mutex;
	int completed_match_threads;
	int done_finding_files;
	int file_list_read_cursor;
	bool cancel_search;
	bool search_completed;

	// search query
	utf8_int8_t *directory_to_search;
	utf8_int8_t *file_filter;
	utf8_int8_t *search_text;
	int max_ts_thread_count;
	int max_file_size;
} ts_search_result;

typedef struct t_ts_file_match
{
	ts_found_file* file;
	int line_nr;
	int word_match_offset; // nr of bytes, not codepoints.
	int word_match_length; // nr of bytes, not codepoints.
	utf8_int8_t *line_info;
} ts_file_match;

typedef struct t_ts_text_match
{
	int line_nr;
	int word_offset;
	int word_match_len;
	utf8_int8_t *line_start;
	utf8_int8_t *line_info;
} ts_text_match;

extern ts_search_result* current_search_result;

ts_array 			ts_get_filters(utf8_int8_t *pattern);
int 				ts_filter_matches(ts_array *filters, utf8_int8_t *string, utf8_int8_t **matched_filter);
int 				ts_string_match(utf8_int8_t *first, utf8_int8_t *second);
ts_search_result* 	ts_create_empty_search_result();
bool 				ts_string_contains(utf8_int8_t *text_to_search, utf8_int8_t *text_to_find, ts_array *text_matches);
void 				ts_start_search(utf8_int8_t* path, utf8_int8_t* filter, utf8_int8_t* query);

#endif