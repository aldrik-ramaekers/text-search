#include "export.h"
#include "array.h"
#include "config.h"
#include "../imfiledialog/ImFileDialog.h"
#include <stdio.h>
#include <inttypes.h>

export_result last_export_result = EXPORT_NONE;

#ifndef _WIN32
#include <errno.h>
int fopen_s(FILE **f, const char *name, const char *mode) {
    int ret = 0;
    assert(f);
    *f = fopen(name, mode);
    /* Can't be sure about 1-to-1 mapping of errno and MS' errno_t */
    if (!*f)
        ret = errno;
    return ret;
}
#endif

bool ts_str_has_extension(const utf8_int8_t *str, const utf8_int8_t *suffix)
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
	if (str == NULL) return buffer;

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
	fprintf(write_file, "\"casesensitive\": %u,\n", result->respect_capitalization);
	fprintf(write_file, "\"match_count\": %u,\n", result->match_count);
	fprintf(write_file, "\"file_count\": %u,\n", result->file_count);
	fprintf(write_file, "\"timestamp\": %" PRId64 ",\n", result->timestamp);

	fprintf(write_file, "\"files\": [\n");

	// Empty files.
	for (uint32_t i = 0; i < result->files.length; i++) {
		ts_found_file* file = *(ts_found_file **)ts_array_at(&result->files, i);
		if (file->match_count != 0) continue;
		fprintf(write_file, "{\n");
		fprintf(write_file, "\"path\": \"%s\",\n", _json_escape_str(file->path, escape_buffer, escape_size));

		fprintf(write_file, "\"matches\": []}\n");

		if (result->matches.length) fprintf(write_file, ",\n");
	}

	// Files with matches.
	bool first_match_of_file = true;
	ts_found_file* prev_file = NULL;
	for (uint32_t i = 0; i < result->matches.length; i++) {
		ts_file_match* match = (ts_file_match*)ts_array_at(&result->matches, i);

		if (match->file != prev_file) {
			first_match_of_file = true;
			if (prev_file != NULL) {
				fprintf(write_file, "]\n");
				if (i == result->files.length-1) fprintf(write_file, "}\n"); else fprintf(write_file, "},\n");
			}
			prev_file = match->file;

			fprintf(write_file, "{\n");
			fprintf(write_file, "\"path\": \"%s\",\n", _json_escape_str(match->file->path, escape_buffer, escape_size));
			fprintf(write_file, "\"matches\": [\n");
		}

		if (!first_match_of_file) fprintf(write_file, ",\n");
		first_match_of_file = false;
		fprintf(write_file, "{\n");
		fprintf(write_file, "\"line_nr\": %u,\n", match->line_nr);
		fprintf(write_file, "\"match_length\": %zu,\n", match->word_match_length);
		fprintf(write_file, "\"match_offset\": %zu,\n", match->word_match_offset);
		fprintf(write_file, "\"line\": \"%s\"\n", _json_escape_str(match->line_info, escape_buffer, escape_size));
		fprintf(write_file, "}\n");
	}
	fprintf(write_file, "]\n");
	fprintf(write_file, "}\n");

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
	fprintf(write_file, "CASESENSITIVE,%u\n", result->respect_capitalization);
	fprintf(write_file, "MATCH_COUNT,%u\n", result->match_count);
	fprintf(write_file, "FILE_COUNT,%u\n", result->file_count);
	fprintf(write_file, "TIMESTAMP,%" PRId64 "\n", result->timestamp);
	fprintf(write_file, "QUERY,%s\n", result->search_text);
	fprintf(write_file, "FILTER,%s\n", result->file_filter);

	// Empty files.
	for (uint32_t i = 0; i < result->files.length; i++) {
		ts_found_file* file = *(ts_found_file **)ts_array_at(&result->files, i);
		if (file->match_count != 0) continue;
		fprintf(write_file, "FILE,%s\n", file->path);
	}

	// Files with matches.
	ts_found_file* prev_file = NULL;
	for (uint32_t i = 0; i < result->matches.length; i++) {
		ts_file_match* match = (ts_file_match*)ts_array_at(&result->matches, i);

		if (match->file != prev_file) {
			prev_file = match->file;
			fprintf(write_file, "FILE,%s\n", match->file->path);
		}

		fprintf(write_file, "MATCH,%u,%zu,%zu,%s\n", match->line_nr, match->word_match_length, match->word_match_offset, match->line_info);
	}

	fclose(write_file);
	return true;
}

static utf8_int8_t* _xml_escape_str(utf8_int8_t* str, utf8_int8_t* buffer, size_t buffer_size) {
	memset(buffer, 0, buffer_size);
	if (str == NULL) return buffer;

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
	fprintf(write_file, "<CASESENSITIVE>%u</CASESENSITIVE>\n", result->respect_capitalization);
	fprintf(write_file, "<MATCH_COUNT>%u</MATCH_COUNT>\n", result->match_count);
	fprintf(write_file, "<FILE_COUNT>%u</FILE_COUNT>\n", result->file_count);
	fprintf(write_file, "<TIMESTAMP>%" PRId64 "</TIMESTAMP>\n", result->timestamp);

	// Empty files.
	for (uint32_t i = 0; i < result->files.length; i++) {
		ts_found_file* file = *(ts_found_file **)ts_array_at(&result->files, i);
		if (file->match_count != 0) continue;
		fprintf(write_file, "<FILE>\n");
		fprintf(write_file, "<PATH>%s</PATH>\n", _xml_escape_str(file->path, escape_buffer, escape_size));
		fprintf(write_file, "</FILE>\n");
	}

	// Files with matches.
	ts_found_file* prev_file = NULL;
	for (uint32_t i = 0; i < result->matches.length; i++) {
		ts_file_match* match = (ts_file_match*)ts_array_at(&result->matches, i);

		if (match->file != prev_file) {
			if (prev_file != NULL) {
				fprintf(write_file, "</FILE>\n");
			}
			prev_file = match->file;

			fprintf(write_file, "<FILE>\n");
			fprintf(write_file, "<PATH>%s</PATH>\n", _xml_escape_str(match->file->path, escape_buffer, escape_size));
		}

		fprintf(write_file, "<MATCH>\n");
		fprintf(write_file, "<LINENR>%u</LINENR>\n", match->line_nr);
		fprintf(write_file, "<MATCH_LENGTH>%zu</MATCH_LENGTH>\n", match->word_match_length);
		fprintf(write_file, "<MATCH_OFFSET>%zu</MATCH_OFFSET>\n", match->word_match_offset);
		fprintf(write_file, "<LINE>%s</LINE>\n", _xml_escape_str(match->line_info, escape_buffer, escape_size));
		fprintf(write_file, "</MATCH>\n");
	}
	fprintf(write_file, "</FILE>\n");

	fprintf(write_file, "</SEARCHRESULT>\n");

	fclose(write_file);
	return true;
}

struct t_export_thread_args {
	ts_search_result* result; 
	const utf8_int8_t* path;
};

static void* _ts_export_thread(void* args) {
	struct t_export_thread_args* arg = (struct t_export_thread_args*)args;

	if (ts_str_has_extension(arg->path, ".json")) {
		_ts_export_json(arg->result, arg->path);
	}
	if (ts_str_has_extension(arg->path, ".csv")) {
		_ts_export_csv(arg->result, arg->path);
	}
	if (ts_str_has_extension(arg->path, ".xml")) {
		_ts_export_xml(arg->result, arg->path);
	}

	arg->result->is_saving = false;
	free(arg);

	return 0;
}

export_result ts_export_result(ts_search_result* result, const utf8_int8_t* path) {
	if (result == NULL || path == NULL) return EXPORT_NO_RESULT;
	if (!result->search_completed) return EXPORT_SEARCH_ACTIVE;
	if (result->is_saving) return EXPORT_SAVE_PENDING;
	result->is_saving = true;

	// Set titlebar name.
	utf8_int8_t new_name[MAX_INPUT_LENGTH];
	snprintf(new_name, MAX_INPUT_LENGTH, "Text-Search > %s", path);
	ts_platform_set_window_title(new_name);

	struct t_export_thread_args* args = (struct t_export_thread_args*)malloc(sizeof(struct t_export_thread_args));
	if (!args) exit_oom();
	args->result = result;
	args->path = path;

	ts_thread_start(_ts_export_thread, args);

	return EXPORT_NONE;
}

void ts_create_export_popup(int window_w, int window_h) {
	// File exporting.
	if (ifd::FileDialog::Instance().IsDone("FileSaveAsDialog", window_w, window_h)) {
		if (ifd::FileDialog::Instance().HasResult()) {
			std::string res = ifd::FileDialog::Instance().GetResult().u8string();
			last_export_result = ts_export_result(current_search_result, (const utf8_int8_t *)res.c_str());
			utf8ncpy(save_path, (const utf8_int8_t *)res.c_str(), sizeof(save_path));
		}
		ifd::FileDialog::Instance().Close();
	}


	if (last_export_result != EXPORT_NONE) {
		ImGui::OpenPopup("Export Failed");
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	}

	// export error popup
	if (ImGui::BeginPopupModal("Export Failed", (bool*)&last_export_result, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
		ImGui::SetWindowSize({300, 0});

		switch (last_export_result)
		{
			case EXPORT_NO_RESULT: ImGui::Text("No results to export"); break;
			case EXPORT_SEARCH_ACTIVE: ImGui::Text("Can't export while save is active"); break;
			case EXPORT_SAVE_PENDING: ImGui::Text("Export is pending"); break;
		
		default:
			break;
		}

		ImGui::Dummy({0, 20});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Button("Close")) {
			last_export_result = EXPORT_NONE;
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}
}