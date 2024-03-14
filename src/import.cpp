#include "import.h"
#include "search.h"
#include "export.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
int fopen_s(FILE **f, const char *name, const char *mode); // defined in export.cpp
#endif

static utf8_int8_t* _ts_str_find(utf8_int8_t* text, utf8_int8_t token) {
	utf8_int32_t ch;
	while ((text = utf8codepoint(text, &ch)) && ch) {
		if (ch == token) return text;
	}
	return NULL;
}

#define fscanf_required(_file, _format, _expect, ...) \
	if (fscanf(_file, _format, __VA_ARGS__) != _expect) { return false; }

static bool _ts_import_csv_v1(ts_search_result* result, FILE *read_file) {
	// Read setup info.
	fscanf_required(read_file, "PATH,%s\n", 1, (char*)result->directory_to_search);
	fscanf_required(read_file, "FILTER,%s\n", 1, (char*)result->file_filter);
	fscanf_required(read_file, "QUERY,%s\n", 1, (char*)result->search_text);
	fscanf_required(read_file, "CASESENSITIVE,%d\n", 1, (int*)&result->respect_capitalization);
	fscanf_required(read_file, "MATCH_COUNT,%u\n", 1, &result->match_count);
	fscanf_required(read_file, "FILE_COUNT,%u\n", 1, &result->file_count);
	fscanf_required(read_file, "TIMESTAMP,%llu\n", 1, &result->timestamp);

	utf8ncpy(path_buffer, result->directory_to_search, MAX_INPUT_LENGTH);
	utf8ncpy(filter_buffer, result->file_filter, MAX_INPUT_LENGTH);
	utf8ncpy(query_buffer, result->search_text, MAX_INPUT_LENGTH);

	result->filters = ts_get_filters(result->file_filter);
	if (utf8len(result->search_text) == 0) {
		result->search_text = nullptr;
	}

	// Read results
	ts_found_file* current_file = 0;
	ts_found_file* next_file = (ts_found_file*)ts_memory_bucket_reserve(&result->memory, sizeof(ts_found_file));
	next_file->path = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
	
	ts_file_match match;
	match.line_info = (char *)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
	memset(match.line_info, 0, MAX_INPUT_LENGTH);

	utf8_int8_t line_buffer[MAX_INPUT_LENGTH];
	while(fgets(line_buffer, MAX_INPUT_LENGTH, read_file)) {
		
		// New file start.
		if (sscanf(line_buffer, "FILE,%s", (char*)next_file->path) == 1) { 
			current_file = next_file;
			current_file->match_count = 0;
			current_file->error = 0;
			current_file->collapsed = false;

			next_file = (ts_found_file*)ts_memory_bucket_reserve(&result->memory, sizeof(ts_found_file));
			next_file->path = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);

			ts_array_push_size(&result->files, &current_file, sizeof(ts_found_file*));
		}

		// New match within current_file
		if (current_file && sscanf(line_buffer, "MATCH,%u,%zu,%zu\n", &match.line_nr, &match.word_match_length, &match.word_match_offset) == 3) {
			match.file = current_file;

			utf8_int8_t* iter = line_buffer;
			int count = 0;
			while ((iter = _ts_str_find(iter, ',')) && iter) {
				count++;

				if (count == 4) { // Copy string from here
					utf8ncpy(match.line_info, iter, MAX_INPUT_LENGTH);
					break;
				}
			}

			ts_array_push_size(&result->matches, &match, sizeof(ts_file_match));
			match.line_info = (char *)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
			memset(match.line_info, 0, MAX_INPUT_LENGTH);
		}
	}

	return true;
}

static bool _ts_import_csv(ts_search_result* result, const utf8_int8_t* path) {
	FILE *read_file;
	fopen_s(&read_file, path, "rb");
	if (read_file == NULL) return false;

	int version = -1;
	bool res = false;
	if (fscanf(read_file, "VERSION,%d\n", &version) != 1) goto done;
	switch(version) {
		case 1: res = _ts_import_csv_v1(result, read_file); break;
		default: break;
	}

	done:
	fclose(read_file);
	return res;
}

ts_search_result* ts_import_result(const utf8_int8_t* path) {
	ts_search_result* res = ts_create_empty_search_result();
	res->done_finding_files = false;
	res->search_completed = false;
	res->cancel_search = false;
	
	if (ts_str_has_extension(path, ".csv")) {
		_ts_import_csv(res, path);
	}

	res->done_finding_files = true;
	res->search_completed = true;

	return res;
}