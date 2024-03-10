#ifndef INCLUDE_SEARCH
#define INCLUDE_SEARCH

#include "array.h"
#include "memory_bucket.h"
#include "../utf8.h"

typedef struct t_ts_found_file
{
	utf8_int8_t *path;
	int match_count;
	int error;
	bool collapsed;
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
	uint64_t timestamp;

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
	uint64_t max_file_size;
	bool respect_capitalization;
} ts_search_result;

typedef struct t_ts_file_match
{
	ts_found_file* file;
	int line_nr;
	size_t word_match_offset; // nr of bytes, not codepoints.
	size_t word_match_length; // nr of bytes, not codepoints.
	utf8_int8_t *line_info;
} ts_file_match;

typedef struct t_ts_text_match
{
	int line_nr;
	size_t word_offset;
	size_t word_match_len;
	utf8_int8_t *line_start;
	utf8_int8_t *line_info;
} ts_text_match;

extern ts_search_result* current_search_result;

ts_array 			ts_get_filters(utf8_int8_t *pattern);
size_t 				ts_filter_matches(ts_array *filters, utf8_int8_t *string, utf8_int8_t **matched_filter);
int 				ts_string_match(utf8_int8_t *first, utf8_int8_t *second);
ts_search_result* 	ts_create_empty_search_result();
bool 				ts_string_contains(utf8_int8_t *text_to_search, utf8_int8_t *text_to_find, ts_array *text_matches, bool respect_capitalization);
void 				ts_start_search(utf8_int8_t *path, utf8_int8_t *filter, utf8_int8_t *query, int thread_count, int max_file_size, bool respect_capitalization);
void				ts_destroy_result(ts_search_result* result);

#endif