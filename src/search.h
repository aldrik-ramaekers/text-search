#pragma once

#include "array.h"
#include "memory_bucket.h"
#include "../utf8.h"

typedef struct t_ts_found_file
{
	utf8_int8_t *path;
	size_t file_size;
	uint32_t match_count;
	uint16_t error;
	bool collapsed;
} ts_found_file;

typedef struct t_ts_search_result
{
	// data
	ts_array files;
	ts_array matches;
	ts_array filters;
	uint32_t match_count;
	uint32_t file_count;
	ts_memory_bucket memory;
	struct t_ts_search_result* prev_result;
	uint64_t timestamp;

	// thread syncing
	ts_mutex mutex;
	uint16_t completed_match_threads;
	bool done_finding_files;
	uint32_t file_list_read_cursor;
	bool cancel_search;
	bool search_completed;
	bool is_saving;

	// search query
	utf8_int8_t *directory_to_search;
	utf8_int8_t *file_filter;
	utf8_int8_t *search_text;
	uint16_t max_ts_thread_count;
	uint32_t max_file_size;
	bool respect_capitalization;
} ts_search_result;

typedef struct t_ts_file_match
{
	ts_found_file* file;
	uint32_t line_nr;
	size_t word_match_offset; // nr of bytes, not codepoints.
	size_t word_match_length; // nr of bytes, not codepoints.
	utf8_int8_t *line_info;
} ts_file_match;

typedef struct t_ts_text_match
{
	uint32_t line_nr;
	size_t word_offset;
	size_t word_match_len;
	utf8_int8_t *line_start;
	utf8_int8_t *line_info;
} ts_text_match;

extern ts_search_result* current_search_result;

ts_array 			ts_get_filters(utf8_int8_t *pattern);
size_t 				ts_filter_matches(ts_array *filters, utf8_int8_t *string, utf8_int8_t **matched_filter);
uint32_t 			ts_string_match(utf8_int8_t *first, utf8_int8_t *second);
ts_search_result* 	ts_create_empty_search_result();
bool 				ts_string_contains(utf8_int8_t *text_to_search, utf8_int8_t *text_to_find, ts_array *text_matches, bool respect_capitalization);
void 				ts_start_search(utf8_int8_t *path, utf8_int8_t *filter, utf8_int8_t *query, uint16_t thread_count, uint32_t max_file_size, bool respect_capitalization);
void				ts_destroy_result(ts_search_result* result);
