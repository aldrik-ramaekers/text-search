#include "import.h"
#include "search.h"
#include "export.h"
#include "../imfiledialog/ImFileDialog.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

import_result last_import_result = IMPORT_NONE;

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
	if (fscanf(_file, _format, __VA_ARGS__) != _expect) { return IMPORT_INVALID_DATA; }

static import_result _ts_import_csv_v1(ts_search_result* result, FILE *read_file) {
	// Required query info.
	fscanf_required(read_file, "PATH,%s\n", 1, (char*)result->directory_to_search);
	fscanf_required(read_file, "CASESENSITIVE,%d\n", 1, (int*)&result->respect_capitalization);
	fscanf_required(read_file, "MATCH_COUNT,%u\n", 1, &result->match_count);
	fscanf_required(read_file, "FILE_COUNT,%u\n", 1, &result->file_count);
	fscanf_required(read_file, "TIMESTAMP,%" PRId64 "\n", 1, &result->timestamp);

	utf8ncpy(path_buffer, result->directory_to_search, MAX_INPUT_LENGTH);
	utf8ncpy(filter_buffer, "", MAX_INPUT_LENGTH);
	utf8ncpy(query_buffer, "", MAX_INPUT_LENGTH);

	// Read results
	ts_found_file* current_file = 0;
	ts_found_file* next_file = (ts_found_file*)ts_memory_bucket_reserve(&result->memory, sizeof(ts_found_file));
	next_file->path = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
	
	ts_file_match match;
	match.line_info = (char *)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
	memset(match.line_info, 0, MAX_INPUT_LENGTH);

	utf8_int8_t line_buffer[MAX_INPUT_LENGTH];
	while(fgets(line_buffer, MAX_INPUT_LENGTH, read_file)) {
		
		// Optional query info.
		if (sscanf(line_buffer, "FILTER,%s", (char*)result->file_filter) == 1) { 
			utf8ncpy(filter_buffer, result->file_filter, MAX_INPUT_LENGTH);
			if (utf8len(result->search_text) == 0) {
				result->search_text = NULL;
			}
		}
		if (sscanf(line_buffer, "QUERY,%s", (char*)result->search_text) == 1) { 
			utf8ncpy(query_buffer, result->search_text, MAX_INPUT_LENGTH);
		}
		
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
		else if (current_file && sscanf(line_buffer, "MATCH,%u,%zu,%zu\n", &match.line_nr, &match.word_match_length, &match.word_match_offset) == 3) {
			match.file = current_file;
			match.file->match_count++;

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
		else {
			// Invalid data. skip.
		}
	}

	result->filters = ts_get_filters(result->file_filter);

	return IMPORT_NONE;
}

static import_result _ts_import_csv(ts_search_result* result, const utf8_int8_t* path) {
	FILE *read_file;
	fopen_s(&read_file, path, "rb");
	if (read_file == NULL) return IMPORT_FILE_ERROR;

	int version = -1;
	import_result res = IMPORT_NONE;
	if (fscanf(read_file, "VERSION,%d\n", &version) != 1) goto done;
	switch(version) {
		case 1: res = _ts_import_csv_v1(result, read_file); break;
		default: res = IMPORT_INVALID_VERSION; break;
	}

	done:
	fclose(read_file);
	return res;
}

struct t_import_thread_args {
	ts_search_result* result; 
	const utf8_int8_t* path;
};

static void* _ts_import_thread(void* args) {
	struct t_import_thread_args* arg = (struct t_import_thread_args*)args;

	if (ts_str_has_extension(arg->path, ".csv")) {
		last_import_result = _ts_import_csv(arg->result, arg->path);
	}

	arg->result->done_finding_files = true;
	arg->result->search_completed = true;

	// Destroy previous result.
	if (arg->result->prev_result) {
		while (!arg->result->prev_result->search_completed || arg->result->prev_result->is_saving) {
			ts_thread_sleep(10);
		}
		ts_destroy_result(arg->result->prev_result);
		arg->result->prev_result = NULL;
	}
	
	free(arg);

	return 0;
}

ts_search_result* ts_import_result(const utf8_int8_t* path) {
	ts_search_result* res = ts_create_empty_search_result();
	res->done_finding_files = false;
	res->search_completed = false;
	res->cancel_search = false;

	// Set titlebar name.
	utf8_int8_t new_name[MAX_INPUT_LENGTH];
	snprintf(new_name, MAX_INPUT_LENGTH, "Text-Search > %s", path);
	ts_platform_set_window_title(new_name);

	if (res->prev_result) res->prev_result->cancel_search = true;
	
	current_search_result = res; // set this now because old result will be destroyed in import thread.
	
	struct t_import_thread_args* args = (struct t_import_thread_args*)malloc(sizeof(struct t_import_thread_args));
	if (!args) exit_oom();
	args->result = res;
	args->path = path;

	ts_thread_start(_ts_import_thread, args);

	return res;
}

void ts_create_import_popup(int window_w, int window_h) {
	// File importing dialog.
	if (ifd::FileDialog::Instance().IsDone("FileOpenDialog", window_w, window_h)) {
		if (ifd::FileDialog::Instance().HasResult()) {
			std::string res = ifd::FileDialog::Instance().GetResult().u8string();
			utf8ncpy(save_path, (const utf8_int8_t *)res.c_str(), sizeof(save_path));
			current_search_result = ts_import_result(save_path);
		}
		ifd::FileDialog::Instance().Close();
	}

	if (last_import_result != IMPORT_NONE) {
		ImGui::OpenPopup("Import Failed");
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	}

	// import error popup
	if (ImGui::BeginPopupModal("Import Failed", (bool*)&last_import_result, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
		ImGui::SetWindowSize({300, 0});

		switch (last_import_result)
		{
			case IMPORT_INVALID_DATA: ImGui::Text("File has invalid format"); break;
			case IMPORT_INVALID_VERSION: ImGui::Text("File has unknown version"); break;
			case IMPORT_FILE_ERROR: ImGui::Text("Failed to open file"); break;
		default:
			break;
		}

		ImGui::Dummy({0, 20});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Button("Close")) {
			last_import_result = IMPORT_NONE;
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}
}