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

search_result *create_empty_search_result();
void* destroy_search_result_thread(void *arg);

static void write_yaml_file(char *buffer, search_result *search_result)
{
	array matches = search_result->files;
	char conv_buf[20];
	
	string_append(buffer, "---\n");
	string_append(buffer, "file_list:\n");
	
	for (s32 i = 0; i < matches.length; i++)
	{
		text_match* m = array_at(&matches, i);
		
		string_append(buffer, "  -\n");
		
		string_append(buffer, "    path: \"");
		string_appendf(buffer, m->file.path);
		string_append(buffer, "\"\n");
		
		string_append(buffer, "    matched_filter: \"");
		string_appendf(buffer, m->file.matched_filter);
		string_append(buffer, "\"\n");
		
		string_append(buffer, "    file_error: ");
		string_appendf(buffer, s32_to_string(m->file_error, conv_buf));
		string_append(buffer, "\n");
		
		string_append(buffer, "    line_nr: ");
		string_appendf(buffer, s32_to_string(m->line_nr, conv_buf));
		string_append(buffer, "\n");
		
		string_append(buffer, "    file_size: ");
		string_appendf(buffer, s32_to_string(m->file_size, conv_buf));
		string_append(buffer, "\n");
		
		if (m->line_info)
		{
			string_append(buffer, "    line_info: \"");
			string_appendf(buffer, m->line_info);
			string_append(buffer, "\"\n");
		}
		else
		{
			string_append(buffer, "    line_info: 0\n");
		}
	}
	
}

static void write_xml_file(char *buffer, search_result *search_result)
{
	array matches = search_result->files;
	char conv_buf[20];
	
	string_append(buffer, "<file_list>");
	
	for (s32 i = 0; i < matches.length; i++)
	{
		text_match* m = array_at(&matches, i);
		
		string_append(buffer, "<file>");
		
		string_append(buffer, "<path>");
		string_appendf(buffer, m->file.path);
		string_append(buffer, "</path>");
		
		string_append(buffer, "<matched_filter>");
		string_appendf(buffer, m->file.matched_filter);
		string_append(buffer, "</matched_filter>");
		
		string_append(buffer, "<file_error>");
		string_appendf(buffer, s32_to_string(m->file_error, conv_buf));
		string_append(buffer, "</file_error>");
		
		string_append(buffer, "<line_nr>");
		string_appendf(buffer, s32_to_string(m->line_nr, conv_buf));
		string_append(buffer, "</line_nr>");
		
		string_append(buffer, "<file_size>");
		string_appendf(buffer, s32_to_string(m->file_size, conv_buf));
		string_append(buffer, "</file_size>");
		
		if (m->line_info)
		{
			string_append(buffer, "<line_info>");
			string_appendf(buffer, m->line_info);
			string_append(buffer, "</line_info>");
		}
		else
		{
			string_append(buffer, "<line_info>0</line_info>");
		}
		
		string_append(buffer, "</file>");
	}
	
	string_append(buffer, "</file_list>");
}

static void write_json_file(char *buffer, search_result *search_result)
{
	array matches = search_result->files;
	char conv_buf[20];
	
	string_append(buffer, "{");
	
	// header
	string_append(buffer, "\"search_directory\": \"");
	string_appendf(buffer, search_result->search_directory_buffer);
	string_append(buffer, "\",");
	
	string_append(buffer, "\"filter\": \"");
	string_appendf(buffer, search_result->filter_buffer);
	string_append(buffer, "\",");
	
	string_append(buffer, "\"search_query\": \"");
	string_appendf(buffer, search_result->text_to_find_buffer);
	string_append(buffer, "\",");
	
	string_append(buffer, "\"duration_us\": ");
	string_appendf(buffer, u64_to_string(search_result->find_duration_us, conv_buf));
	string_append(buffer, ",");
	
	string_append(buffer, "\"show_error\": ");
	string_appendf(buffer, s32_to_string(search_result->show_error_message, conv_buf));
	string_append(buffer, ",");
	
	string_append(buffer, "\"file_match_found\": ");
	string_appendf(buffer, s32_to_string(search_result->found_file_matches, conv_buf));
	string_append(buffer, ",");
	
	string_append(buffer, "\"files_searched\": ");
	string_appendf(buffer, s32_to_string(search_result->files_searched, conv_buf));
	string_append(buffer, ",");
	
	string_append(buffer, "\"files_matched\": ");
	string_appendf(buffer, s32_to_string(search_result->files_matched, conv_buf));
	string_append(buffer, ",");
	
	string_append(buffer, "\"query_match_found\": ");
	string_appendf(buffer, s32_to_string(search_result->match_found, conv_buf));
	string_append(buffer, ",");
	
	string_append(buffer, "\"recursive_search\": ");
	string_appendf(buffer, s32_to_string(search_result->match_found, conv_buf));
	string_append(buffer, ",");
	
	string_append(buffer, "\"match_list\": ");
	
	string_append(buffer, "[");
	
	for (s32 i = 0; i < matches.length; i++)
	{
		text_match* m = array_at(&matches, i);
		
		string_append(buffer, "{");
		
		string_append(buffer, "\"path\": \"");
		string_appendf(buffer, m->file.path);
		string_append(buffer, "\",");
		
		string_append(buffer, "\"matched_filter\": \"");
		string_appendf(buffer, m->file.matched_filter);
		string_append(buffer, "\",");
		
		string_append(buffer, "\"file_error\": ");
		string_appendf(buffer, s32_to_string(m->file_error, conv_buf));
		string_append(buffer, ",");
		
		string_append(buffer, "\"line_nr\": ");
		string_appendf(buffer, s32_to_string(m->line_nr, conv_buf));
		string_append(buffer, ",");
		
		string_append(buffer, "\"file_size\": ");
		string_appendf(buffer, s32_to_string(m->file_size, conv_buf));
		string_append(buffer, ",");
		
		if (m->line_info)
		{
			string_append(buffer, "\"line_info\": \"");
			string_appendf(buffer, m->line_info);
			string_append(buffer, "\"");
		}
		else
		{
			string_append(buffer, "\"line_info\": 0");
		}
		
		string_append(buffer, "}");
		
		if (i != matches.length-1)
			string_append(buffer, ",");
	}
	
	string_append(buffer, "]");
	string_append(buffer, "}");
}

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
	args->file_filter = SEARCH_RESULT_AVAILABLE_FORMATS;
	args->start_path = start_path;
	
	platform_open_file_dialog_block(args);
	
	char tmp_dir_buffer[MAX_INPUT_LENGTH];
	get_directory_from_path(tmp_dir_buffer, path_buf);
	
	char tmp_name_buffer[MAX_INPUT_LENGTH];
	get_name_from_path(tmp_name_buffer, path_buf);
	
	if (string_equals(path_buf, "")) return 0;
	if (string_equals(tmp_name_buffer, "")) return 0;
	if (!platform_directory_exists(tmp_dir_buffer)) return 0;
	
	s32 size = matches.length * (MAX_INPUT_LENGTH*10);
	char *buffer = mem_alloc(size);
	memset(buffer, 0, size);
	
	char *file_extension = get_file_extension(path_buf);
	if (string_equals(file_extension, ".json") || string_equals(file_extension, ""))
	{
		write_json_file(buffer, search_result);
	}
	if (string_equals(file_extension, ".xml"))
	{
		write_xml_file(buffer, search_result);
	}
	if (string_equals(file_extension, ".yaml"))
	{
		write_yaml_file(buffer, search_result);
	}
	
	if (string_equals(file_extension, ""))
	{
		strcat(path_buf, ".json");
	}
	
	platform_write_file_content(path_buf, "w", buffer, size);
	
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

static bool read_json_file(char *buffer, s32 size, search_result *search_result)
{
	array matches = search_result->files;
	char conv_buf[100];
	
	text_match new_match;
	
	if (!string_remove(&buffer, "{")) return false;
	
	// header
	if (!string_remove(&buffer, "\"search_directory\": \"")) return false;
	char *search_directory = string_get_json_literal(&buffer, conv_buf);
	if (!string_remove(&buffer, "\",")) return false;
	strcpy(search_result->search_directory_buffer, search_directory);
	
	if (!string_remove(&buffer, "\"filter\": \"")) return false;
	char *filter = string_get_json_literal(&buffer, conv_buf);
	if (!string_remove(&buffer, "\",")) return false;
	strcpy(search_result->filter_buffer, filter);
	
	if (!string_remove(&buffer, "\"search_query\": \"")) return false;
	char *text_query = string_get_json_literal(&buffer, conv_buf);
	if (!string_remove(&buffer, "\",")) return false;
	strcpy(search_result->text_to_find_buffer, text_query);
	
	if (!string_remove(&buffer, "\"duration_us\": ")) return false;
	s32 duration_us = string_get_json_ulong_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	search_result->find_duration_us = duration_us;
	
	if (!string_remove(&buffer, "\"show_error\": ")) return false;
	s32 show_error = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	search_result->show_error_message = show_error;
	
	if (!string_remove(&buffer, "\"file_match_found\": ")) return false;
	s32 found_file_match = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	search_result->found_file_matches = found_file_match;
	
	if (!string_remove(&buffer, "\"files_searched\": ")) return false;
	s32 files_searched = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	search_result->files_searched = files_searched;
	
	if (!string_remove(&buffer, "\"files_matched\": ")) return false;
	s32 files_matched = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	search_result->files_matched = files_matched;
	
	if (!string_remove(&buffer, "\"query_match_found\": ")) return false;
	s32 match_found = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	search_result->match_found = match_found;
	
	if (!string_remove(&buffer, "\"recursive_search\": ")) return false;
	s32 recursive = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	*search_result->recursive_state_buffer = recursive;
	
	if (!string_remove(&buffer, "\"match_list\": ")) return false;
	if (!string_remove(&buffer, "[")) return false;
	
	search_result->search_result_source_dir_len = strlen(search_result->search_directory_buffer);
	
	new_item_found:
	if (!string_remove(&buffer, "{")) return false;
	if (!string_remove(&buffer, "\"path\": \"")) return false;
	char *path = string_get_json_literal(&buffer, conv_buf);
	if (!string_remove(&buffer, "\",")) return false;
	new_match.file.path = memory_bucket_reserve(&search_result->mem_bucket, strlen(path)+1);
	strcpy(new_match.file.path, path);
	
	if (!string_remove(&buffer, "\"matched_filter\": \"")) return false;
	char *filter_matched = string_get_json_literal(&buffer, conv_buf);
	if (!string_remove(&buffer, "\",")) return false;
	new_match.file.matched_filter = memory_bucket_reserve(&search_result->mem_bucket, strlen(filter_matched)+1);
	strcpy(new_match.file.matched_filter, filter_matched);
	
	if (!string_remove(&buffer, "\"file_error\": ")) return false;
	s32 file_error = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	new_match.file_error = file_error;
	
	if (!string_remove(&buffer, "\"line_nr\": ")) return false;
	s32 line_nr = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	new_match.line_nr = line_nr;
	
	if (!string_remove(&buffer, "\"file_size\": ")) return false;
	s32 file_size = string_get_json_number(&buffer);
	if (!string_remove(&buffer, ",")) return false;
	new_match.file_size = file_size;
	
	if (!string_remove(&buffer, "\"line_info\": ")) return false;
	char *line_info = 0;
	if (!string_remove(&buffer, "\""))
	{
		string_get_json_number(&buffer);
		new_match.line_info = 0;
	}
	else
	{
		line_info = string_get_json_literal(&buffer, conv_buf);
		if (!string_remove(&buffer, "\"")) return false;
		
		new_match.line_info = memory_bucket_reserve(&search_result->mem_bucket, strlen(line_info)+1);
		strcpy(new_match.line_info, line_info);
		
	}
	
	if (!string_remove(&buffer, "}")) return false;
	array_push(&search_result->files, &new_match);
	if (string_remove(&buffer, ",")) goto new_item_found;
	if (!string_remove(&buffer, "]")) return false;
	if (!string_remove(&buffer, "}")) return false;
	
	return true;
}

void import_results_from_file(char *path_buf)
{
	char *file_extension = get_file_extension(path_buf);
	if (!string_equals(file_extension, ".json") && !string_equals(file_extension, ".xml") && !string_equals(file_extension, ".yaml"))
	{
		platform_show_message(main_window, localize("invalid_search_result_file"), localize("error_importing_results"));
		return;
	}
	
	scroll_y = 0;
	file_content content = platform_read_file_content(path_buf, "r");
	
	if (!content.content || content.file_error)
	{
		platform_destroy_file_content(&content);
		return;
	}
	
	search_result *new_result = create_empty_search_result();
	search_result *old_result = current_search_result;
	current_search_result = new_result;
	
	thread cleanup_thread = thread_start(destroy_search_result_thread, old_result);
	thread_detach(&cleanup_thread);
	
	if (string_equals(file_extension, ".json"))
	{
		bool result = read_json_file(content.content, content.content_length, new_result);
		if (!result) goto failed_to_load_file;
		
		new_result->walking_file_system = false;
		new_result->done_finding_matches = true;
		new_result->done_finding_files = true;
	}
	
	sprintf(global_status_bar.result_status_text, localize("files_matches_comparison"), current_search_result->files_matched, current_search_result->files.length, current_search_result->find_duration_us/1000.0);
	
	platform_destroy_file_content(&content);
	return;
	
	failed_to_load_file:
	platform_show_message(main_window, localize("invalid_search_result_file"), localize("error_importing_results"));
	platform_destroy_file_content(&content);
}

static void* import_results_d(void *arg)
{
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
	
	import_results_from_file(path_buf);
	return 0;
}

void import_results()
{
	thread thr;
	thr.valid = false;
	
	while (!thr.valid)
		thr = thread_start(import_results_d, 0);
	thread_detach(&thr);
}