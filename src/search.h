#ifndef INCLUDE_SEARCH
#define INCLUDE_SEARCH

#define MAX_INPUT_LENGTH 4096
#define MAX_ERROR_MESSAGE_LENGTH (MAX_INPUT_LENGTH)
#define FILE_RESERVE_COUNT 100000
#define ERROR_RESERVE_COUNT 100

#include "array.h"
#include "memory_bucket.h"
#include "../utf8.h"

typedef struct t_found_file
{
	utf8_int8_t *path;
	int match_count;
} found_file;

typedef struct t_search_result
{
	// data
	array files;
	array matches;
	int match_count;
	int file_count;

	// thread syncing
	mutex mutex;
	int completed_match_threads;
	int done_finding_files;
	int file_list_read_cursor;
	bool cancel_search;

	// search query
	utf8_int8_t *directory_to_search;
	utf8_int8_t *search_text;
	int max_thread_count;
	int max_file_size;
} search_result;

typedef struct t_file_match
{
	found_file* file;
	int line_nr;
	int word_match_offset;
	int word_match_length;
	utf8_int8_t *line_info; // will be null when no match is found
} file_match;

typedef struct t_text_match
{
	int line_nr;
	int word_offset;
	int word_match_len;
	char *line_start;
	char *line_info;
} text_match;

extern search_result* current_search_result;

array get_filters(char *pattern);
int filter_matches(array *filters, char *string, char **matched_filter);
int string_match(char *first, char *second);
search_result *create_empty_search_result();
bool string_contains_ex(char *text_to_search, char *text_to_find, array *text_matches);

void ts_start_search(utf8_int8_t* path, utf8_int8_t* filter, utf8_int8_t* query);

#endif