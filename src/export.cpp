#include "export.h"
#include "array.h"
#include "config.h"
#include <stdio.h>

static bool _str_has_extension(const utf8_int8_t *str, const utf8_int8_t *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static utf8_int8_t* _json_escape_str(utf8_int8_t* str, utf8_int8_t* buffer, size_t buffer_size) {
	memset(buffer, 0, buffer_size);

	utf8_int8_t* buffer_orig = buffer;
	utf8_int32_t ch;
	while ((str = utf8codepoint(str, &ch)) && ch)
	{
		if (ch == '\\') {
			utf8cat(buffer, "\\\\");
			buffer += 2;
		}
		else if (ch == '\"') {
			utf8cat(buffer, "\\\"");
			buffer += 2;
		}
		else {
			buffer = utf8catcodepoint(buffer, ch, buffer_size - (buffer - buffer_orig) - 1);
		}
	}
	return buffer_orig;
}

static bool _ts_export_json(ts_search_result* result, const utf8_int8_t* path) {
	FILE *write_file;
	fopen_s(&write_file, path, "w");
	if (write_file == NULL) return false;

	const size_t escape_size = MAX_INPUT_LENGTH*2; // ballpark.
	utf8_int8_t escape_buffer[escape_size]; 

	fprintf(write_file, "{\n");

	fprintf(write_file, "\"version\": 1,\n");
	fprintf(write_file, "\"path\": \"%s\",\n", _json_escape_str(result->directory_to_search, escape_buffer, escape_size));
	fprintf(write_file, "\"filter\": \"%s\",\n", _json_escape_str(result->file_filter, escape_buffer, escape_size));
	fprintf(write_file, "\"query\": \"%s\",\n", _json_escape_str(result->search_text, escape_buffer, escape_size));
	fprintf(write_file, "\"casesensitive\": %d,\n", result->respect_capitalization);
	fprintf(write_file, "\"match_count\": %d,\n", result->match_count);
	fprintf(write_file, "\"file_count\": %d,\n", result->file_count);
	fprintf(write_file, "\"timestamp\": %llu,\n", result->timestamp);

	fprintf(write_file, "\"files\": [\n");

	for (int i = 0; i < result->files.length; i++) {
		ts_found_file* file = *(ts_found_file **)ts_array_at(&result->files, i);

		fprintf(write_file, "{\n");
		fprintf(write_file, "\"path\": \"%s\",\n", _json_escape_str(file->path, escape_buffer, escape_size));

		fprintf(write_file, "\"matches\": [\n");

		bool first_match_of_file = true;
		for (int i = 0; i < result->matches.length; i++) {
			ts_file_match* match = (ts_file_match*)ts_array_at(&result->matches, i);
			if (match->file != file) continue;

			if (!first_match_of_file) fprintf(write_file, ",\n");
			first_match_of_file = false;
			fprintf(write_file, "{\n");
			fprintf(write_file, "\"line_nr\": %d,\n", match->line_nr);
			fprintf(write_file, "\"match_length\": %zu,\n", match->word_match_length);
			fprintf(write_file, "\"match_offset\": %zu,\n", match->word_match_offset);
			fprintf(write_file, "\"line\": \"%s\"\n", _json_escape_str(match->line_info, escape_buffer, escape_size));
			fprintf(write_file, "}\n");
		}

		fprintf(write_file, "]\n");
		if (i == result->files.length-1) fprintf(write_file, "}\n"); else fprintf(write_file, "},\n");
	}

	fprintf(write_file, "]\n");

	fprintf(write_file, "}\n");

	fclose(write_file);
	return true;
}

static bool _ts_export_csv(ts_search_result* result, const utf8_int8_t* path) {
	FILE *write_file;
	fopen_s(&write_file, path, "w");
	if (write_file == NULL) return false;

	fprintf(write_file, "VERSION,1\n");
	fprintf(write_file, "PATH,%s\n", result->directory_to_search);
	fprintf(write_file, "FILTER,%s\n", result->file_filter);
	fprintf(write_file, "QUERY,%s\n", result->search_text);
	fprintf(write_file, "CASESENSITIVE,%d\n", result->respect_capitalization);
	fprintf(write_file, "MATCH_COUNT,%d\n", result->match_count);
	fprintf(write_file, "FILE_COUNT,%d\n", result->file_count);
	fprintf(write_file, "TIMESTAMP,%llu\n", result->timestamp);

	for (int i = 0; i < result->files.length; i++) {
		ts_found_file* file = *(ts_found_file **)ts_array_at(&result->files, i);
		fprintf(write_file, "FILE,%s\n", file->path);

		for (int i = 0; i < result->matches.length; i++) {
			ts_file_match* match = (ts_file_match*)ts_array_at(&result->matches, i);
			if (match->file != file) continue;
			fprintf(write_file, "MATCH,%d,%zu,%zu,%s\n", match->line_nr, match->word_match_length, match->word_match_offset, match->line_info);
		}
	}

	fclose(write_file);
	return true;
}

static utf8_int8_t* _xml_escape_str(utf8_int8_t* str, utf8_int8_t* buffer, size_t buffer_size) {
	memset(buffer, 0, buffer_size);

	utf8_int8_t* buffer_orig = buffer;
	utf8_int32_t ch;
	while ((str = utf8codepoint(str, &ch)) && ch)
	{
		switch(ch) {
			case '<': utf8cat(buffer, "&lt;"); buffer += 4; break;
			case '>': utf8cat(buffer, "&gt;"); buffer += 4; break;
			case '&': utf8cat(buffer, "&#38;"); buffer += 5; break;
			case '\'': utf8cat(buffer, "&#39;"); buffer += 5; break;
			case '"': utf8cat(buffer, "&#34;"); buffer += 5; break;
			default: buffer = utf8catcodepoint(buffer, ch, buffer_size - (buffer - buffer_orig) - 1);
		}	
	}
	return buffer_orig;
}

static bool _ts_export_xml(ts_search_result* result, const utf8_int8_t* path) {
	FILE *write_file;
	fopen_s(&write_file, path, "w");
	if (write_file == NULL) return false;

	const size_t escape_size = MAX_INPUT_LENGTH*2; // ballpark.
	utf8_int8_t escape_buffer[escape_size]; 

	fprintf(write_file, "<SEARCHRESULT>\n");

	fprintf(write_file, "<VERSION>1</VERSION>\n");
	fprintf(write_file, "<PATH>%s</PATH>\n", _xml_escape_str(result->directory_to_search, escape_buffer, escape_size));
	fprintf(write_file, "<FILTER>%s</FILTER>\n", _xml_escape_str(result->file_filter, escape_buffer, escape_size));
	fprintf(write_file, "<QUERY>%s</QUERY>\n", _xml_escape_str(result->search_text, escape_buffer, escape_size));
	fprintf(write_file, "<CASESENSITIVE>%d</CASESENSITIVE>\n", result->respect_capitalization);
	fprintf(write_file, "<MATCH_COUNT>%d</MATCH_COUNT>\n", result->match_count);
	fprintf(write_file, "<FILE_COUNT>%d</FILE_COUNT>\n", result->file_count);
	fprintf(write_file, "<TIMESTAMP>%llu</TIMESTAMP>\n", result->timestamp);

	for (int i = 0; i < result->files.length; i++) {
		ts_found_file* file = *(ts_found_file **)ts_array_at(&result->files, i);

		fprintf(write_file, "<FILE>\n");
		fprintf(write_file, "<PATH>%s</PATH>\n", _xml_escape_str(file->path, escape_buffer, escape_size));

		for (int i = 0; i < result->matches.length; i++) {
			ts_file_match* match = (ts_file_match*)ts_array_at(&result->matches, i);
			if (match->file != file) continue;

			fprintf(write_file, "<MATCH>\n");
			fprintf(write_file, "<LINENR>%d</LINENR>\n", match->line_nr);
			fprintf(write_file, "<MATCH_LENGTH>%zu</MATCH_LENGTH>\n", match->word_match_length);
			fprintf(write_file, "<MATCH_OFFSET>%zu</MATCH_OFFSET>\n", match->word_match_offset);
			fprintf(write_file, "<LINE>%s</LINE>\n", _xml_escape_str(match->line_info, escape_buffer, escape_size));
			fprintf(write_file, "</MATCH>\n");
		}

		fprintf(write_file, "</FILE>\n");
	}

	fprintf(write_file, "</SEARCHRESULT>\n");

	fclose(write_file);
	return true;
}

bool ts_export_result(ts_search_result* result, const utf8_int8_t* path) {
	if (result == NULL || path == NULL) return false;
	if (!result->search_completed) return false;
	result->is_saving = true;

	if (_str_has_extension(path, ".json")) {
		return _ts_export_json(result, path);
	}
	if (_str_has_extension(path, ".csv")) {
		return _ts_export_csv(result, path);
	}
	if (_str_has_extension(path, ".xml")) {
		return _ts_export_xml(result, path);
	}

	result->is_saving = false;

	return false;
}