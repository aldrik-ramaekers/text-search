/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.

*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.

*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

static void *export_result_d(void *arg)
{
	search_result *search_result = arg;
	
	array matches = search_result->files;
	
	char path_buf[MAX_INPUT_LENGTH];
	path_buf[0] = 0;
	
	char start_path[MAX_INPUT_LENGTH];
	sprintf(start_path, "%s%s", binary_path, "/data/export/");
	
	struct open_dialog_args *args = mem_alloc(sizeof(struct open_dialog_args));
	args->buffer = path_buf;
	args->type = SAVE_FILE;
	args->file_filter = SEARCH_RESULT_FILE_EXTENSION;
	args->start_path = start_path;
	
	platform_open_file_dialog_block(args);
	
	char tmp_dir_buffer[MAX_INPUT_LENGTH];
	get_directory_from_path(tmp_dir_buffer, path_buf);
	
	char tmp_name_buffer[MAX_INPUT_LENGTH];
	get_name_from_path(tmp_name_buffer, path_buf);
	
	if (string_equals(path_buf, "")) return 0;
	if (string_equals(tmp_name_buffer, "")) return 0;
	if (!platform_directory_exists(tmp_dir_buffer)) return 0;
	
	s32 size = ((MAX_INPUT_LENGTH*3) + 53)
		+ matches.length * (matches.entry_size + MAX_INPUT_LENGTH);
	//char *buffer = mem_alloc(size);
	
	text_buffer save_file_buffer = text_buffer_create(size);
	memset(save_file_buffer.data, 0, size);
	
	// write search result info
	buffer_write_string(&save_file_buffer, search_result->search_directory_buffer);
	buffer_write_string(&save_file_buffer, search_result->filter_buffer);
	buffer_write_string(&save_file_buffer, search_result->text_to_find_buffer);
	buffer_write_unsigned(&save_file_buffer, search_result->find_duration_us);
	buffer_write_unsigned(&save_file_buffer, search_result->show_error_message);
	buffer_write_unsigned(&save_file_buffer, search_result->found_file_matches);
	buffer_write_signed(&save_file_buffer, search_result->files_searched);
	buffer_write_signed(&save_file_buffer, search_result->files_matched);
	buffer_write_signed(&save_file_buffer, search_result->search_result_source_dir_len);
	buffer_write_unsigned(&save_file_buffer, search_result->match_found);
	buffer_write_unsigned(&save_file_buffer, *search_result->recursive_state_buffer);
	
	for (s32 i = 0; i < matches.length; i++)
	{
		text_match* m = array_at(&matches, i);
		
		buffer_write_string(&save_file_buffer, m->file.path);
		buffer_write_string(&save_file_buffer, m->file.matched_filter);
		buffer_write_signed(&save_file_buffer, m->file_error);
		buffer_write_signed(&save_file_buffer, m->match_count);
		buffer_write_signed(&save_file_buffer, m->file_size);
		
		if (m->line_info)
			buffer_write_string(&save_file_buffer, m->line_info);
		else
			buffer_write_string(&save_file_buffer, "");
	}
	
	if (!string_contains(path_buf, SEARCH_RESULT_FILE_EXTENSION))
	{
		strcat(path_buf, ".tts");
	}
	
	platform_write_file_content(path_buf, "w", save_file_buffer.data, size);
	text_buffer_destroy(&save_file_buffer);
	
	return 0;
}

bool export_results(search_result *search_result)
{
	thread thr;
	thr.valid = false;
	
	while (!thr.valid)
		thr = thread_start(export_result_d, search_result);
	thread_detach(&thr);
	
	return true;
}

void import_results_from_file(search_result *search_result, char *path_buf)
{
	memory_bucket_collection bucket = memory_bucket_init(megabytes(1));
	
	if (!string_contains(path_buf, SEARCH_RESULT_FILE_EXTENSION))
	{
		platform_show_message(main_window, localize("invalid_search_result_file"), localize("error_importing_results"));
		return;
	}
	
	//platform_destroy_list_file_result(&global_search_result.files);
	scroll_y = 0;
	
	// sprintf(buffer, "%.16lu\n%.1d\n%.1d\n%.8d\n%.8d\n%.8d\n%.1d\n",
	file_content content = platform_read_file_content(path_buf, "r");
	
	if (!content.content || content.file_error)
	{
		platform_destroy_file_content(&content);
		return;
	}
	
	text_buffer save_file_buffer;
	save_file_buffer.read_cursor = 0;
	save_file_buffer.data = content.content;
	save_file_buffer.buffer_size = content.content_length;
	save_file_buffer.len = content.content_length;
	
	// read search result info
	buffer_read_string(&save_file_buffer, search_result->search_directory_buffer);
	buffer_read_string(&save_file_buffer, search_result->filter_buffer);
	buffer_read_string(&save_file_buffer, search_result->text_to_find_buffer);
	search_result->find_duration_us = buffer_read_unsigned(&save_file_buffer);
	search_result->show_error_message = buffer_read_unsigned(&save_file_buffer);
	search_result->found_file_matches = buffer_read_unsigned(&save_file_buffer);
	search_result->files_searched = buffer_read_signed(&save_file_buffer);
	search_result->files_matched = buffer_read_signed(&save_file_buffer);
	search_result->search_result_source_dir_len = buffer_read_signed(&save_file_buffer);
	search_result->match_found = buffer_read_unsigned(&save_file_buffer);
	*search_result->recursive_state_buffer = buffer_read_unsigned(&save_file_buffer);
	
	text_match match;
	while (!buffer_done_reading(&save_file_buffer))
	{
		match.file.path = memory_bucket_reserve(&bucket, MAX_INPUT_LENGTH);
		buffer_read_string(&save_file_buffer, match.file.path);
		
		match.file.matched_filter = memory_bucket_reserve(&bucket, MAX_INPUT_LENGTH);
		buffer_read_string(&save_file_buffer, match.file.matched_filter);
		
		match.file_error = buffer_read_signed(&save_file_buffer);
		match.match_count = buffer_read_signed(&save_file_buffer);
		match.file_size = buffer_read_signed(&save_file_buffer);
		
		match.line_info = memory_bucket_reserve(&bucket, MAX_INPUT_LENGTH);
		buffer_read_string(&save_file_buffer, match.line_info);
		
		array_push(&search_result->files, &match);
	}
	
	sprintf(global_status_bar.result_status_text, localize("files_matches_comparison"), current_search_result->files_matched, current_search_result->files.length, current_search_result->find_duration_us/1000.0);
	
	memory_bucket_destroy(&bucket);
	platform_destroy_file_content(&content);
}

static void* import_results_d(void *arg)
{
	search_result *search_result = arg;
	
	array matches = search_result->files;
	
	char path_buf[MAX_INPUT_LENGTH];
	path_buf[0] = 0;
	
	char start_path[MAX_INPUT_LENGTH];
	sprintf(start_path, "%s%s", binary_path, "/data/export/");
	
	struct open_dialog_args *args = mem_alloc(sizeof(struct open_dialog_args));
	args->buffer = path_buf;
	args->type = OPEN_FILE;
	args->file_filter = SEARCH_RESULT_FILE_EXTENSION;
	args->start_path = start_path;
	
	platform_open_file_dialog_block(args);
	
	if (string_equals(path_buf, "")) return 0;
	if (!platform_file_exists(path_buf)) return 0;
	
	import_results_from_file(search_result, path_buf);
	return 0;
}

void import_results(search_result *result)
{
	thread thr;
	thr.valid = false;
	
	while (!thr.valid)
		thr = thread_start(import_results_d, result);
	thread_detach(&thr);
}