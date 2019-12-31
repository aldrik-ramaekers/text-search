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

static void write_json_file(char *buffer, s32 length, search_result *search_result)
{
	array matches = search_result->matches;
	
	cJSON *result = cJSON_CreateObject();
	if (cJSON_AddStringToObject(result, "search_directory", 
								search_result->search_directory_buffer) == NULL)
		return;
	
	if (cJSON_AddStringToObject(result, "filter", 
								search_result->filter_buffer) == NULL)
		return;
	
	if (cJSON_AddStringToObject(result, "search_query", 
								search_result->text_to_find_buffer) == NULL)
		return;
	
	if (cJSON_AddNumberToObject(result, "duration_us", 
								search_result->find_duration_us) == NULL)
		return;
	
	if (cJSON_AddNumberToObject(result, "show_error", 
								search_result->show_error_message) == NULL)
		return;
	
	if (cJSON_AddNumberToObject(result, "file_match_found", 
								search_result->found_file_matches) == NULL)
		return;
	
	if (cJSON_AddNumberToObject(result, "files_searched", 
								search_result->files_searched) == NULL)
		return;
	
	if (cJSON_AddNumberToObject(result, "files_matched", 
								search_result->files_matched) == NULL)
		return;
	
	if (cJSON_AddNumberToObject(result, "query_match_found", 
								search_result->match_found) == NULL)
		return;
	
	if (cJSON_AddNumberToObject(result, "recursive_search", 
								search_result->is_recursive) == NULL)
		return;
	
	cJSON *match_list = cJSON_AddArrayToObject(result, "match_list");
	
	if (!match_list) return;
	
	for (s32 i = 0; i < matches.length; i++)
	{
		file_match* m = array_at(&matches, i);
		
		cJSON *item = cJSON_CreateObject();
		
		if (cJSON_AddStringToObject(item, "path", 
									m->file.path) == NULL)
			return;
		
		if (cJSON_AddStringToObject(item, "matched_filter", 
									m->file.matched_filter) == NULL)
			return;
		
		if (cJSON_AddNumberToObject(item, "file_error", 
									m->file_error) == NULL)
			return;
		
		if (cJSON_AddNumberToObject(item, "line_nr", 
									m->line_nr) == NULL)
			return;
		
		if (cJSON_AddNumberToObject(item, "file_size", 
									m->file_size) == NULL)
			return;
		
		if (m->line_info)
		{
			if (cJSON_AddStringToObject(item, "line_info", 
										m->line_info) == NULL)
				return;
		}
		else
		{
			if (cJSON_AddNumberToObject(item, "line_info", 0) == NULL)
				return;
		}
		
		cJSON_AddItemToArray(match_list, item);
	}
	
	cJSON_PrintPreallocated(result, buffer, length, true);
	cJSON_Delete(result);
}

static void *export_result_d(void *arg)
{
	search_result *search_result = arg;
	
	array matches = search_result->files;
	
	char path_buf[MAX_INPUT_LENGTH];
	path_buf[0] = 0;
	
	char start_path[MAX_INPUT_LENGTH];
	snprintf(start_path, MAX_INPUT_LENGTH, "%s%s", binary_path, "");
	
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
	
	s32 size = matches.length * (MAX_INPUT_LENGTH*10);
	char *buffer = mem_alloc(size);
	memset(buffer, 0, size);
	
	char *file_extension = get_file_extension(path_buf);
	if (string_equals(file_extension, ".json") || string_equals(file_extension, ""))
	{
		write_json_file(buffer, size, search_result);
	}
	
	if (string_equals(file_extension, ""))
	{
		string_appendn(path_buf, ".json", MAX_INPUT_LENGTH);
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
	cJSON *result = cJSON_Parse(buffer);
	if (!result) return false;
	
	cJSON *search_directory = cJSON_GetObjectItemCaseSensitive(result, "search_directory");
	string_copyn(search_result->search_directory_buffer, search_directory->valuestring, MAX_INPUT_LENGTH);
	
	cJSON *filter = cJSON_GetObjectItemCaseSensitive(result, "filter");
	string_copyn(search_result->filter_buffer, filter->valuestring, MAX_INPUT_LENGTH);
	
	cJSON *search_query = cJSON_GetObjectItemCaseSensitive(result, "search_query");
	string_copyn(search_result->text_to_find_buffer, search_query->valuestring, MAX_INPUT_LENGTH);
	
	cJSON *duration_us = cJSON_GetObjectItemCaseSensitive(result, "duration_us");
	search_result->find_duration_us = duration_us->valueint;
	
	cJSON *show_error = cJSON_GetObjectItemCaseSensitive(result, "show_error");
	search_result->show_error_message = show_error->valueint;
	
	cJSON *file_match_found = cJSON_GetObjectItemCaseSensitive(result, "file_match_found");
	search_result->found_file_matches = file_match_found->valueint;
	
	cJSON *files_searched = cJSON_GetObjectItemCaseSensitive(result, "files_searched");
	search_result->files_searched = files_searched->valueint;
	
	cJSON *files_matched = cJSON_GetObjectItemCaseSensitive(result, "files_matched");
	search_result->files_matched = files_matched->valueint;
	
	cJSON *query_match_found = cJSON_GetObjectItemCaseSensitive(result, "query_match_found");
	search_result->match_found = query_match_found->valueint;
	
	cJSON *recursive = cJSON_GetObjectItemCaseSensitive(result, "recursive_search");
	search_result->is_recursive = recursive->valueint;
	*search_result->recursive_buffer = search_result->is_recursive;
	
	search_result->search_result_source_dir_len = strlen(search_result->search_directory_buffer);
	
	cJSON *file_list = cJSON_GetObjectItem(result, "match_list");
	cJSON *file;
	cJSON_ArrayForEach(file, file_list)
	{
		file_match new_match;
		
		////
		cJSON *path = cJSON_GetObjectItem(file, "path");
		new_match.file.path = memory_bucket_reserve(&search_result->mem_bucket, strlen(path->valuestring)+1);
		string_copyn(new_match.file.path, path->valuestring, strlen(path->valuestring)+1);
		
		////
        cJSON *matched_filter = cJSON_GetObjectItem(file, "matched_filter");
		new_match.file.matched_filter = memory_bucket_reserve(&search_result->mem_bucket, strlen(matched_filter->valuestring)+1);
		string_copyn(new_match.file.matched_filter, matched_filter->valuestring, strlen(matched_filter->valuestring)+1);
		
		////
		cJSON *file_error = cJSON_GetObjectItem(file, "file_error");
		new_match.file_error = file_error->valueint;
		
		////
		cJSON *line_nr = cJSON_GetObjectItem(file, "line_nr");
		new_match.line_nr = line_nr->valueint;
		
		////
		cJSON *file_size = cJSON_GetObjectItem(file, "file_size");
		new_match.file_size = file_size->valueint;
		
		////
		cJSON *line_info = cJSON_GetObjectItem(file, "line_info");
		if (cJSON_IsString(line_info))
		{
			new_match.line_info = memory_bucket_reserve(&search_result->mem_bucket, strlen(line_info->valuestring)+1);
			string_copyn(new_match.line_info, line_info->valuestring, strlen(line_info->valuestring)+1);
			search_result->match_found = true;
		}
		else
		{
			new_match.line_info = 0;
		}
		
		array_push(&search_result->matches, &new_match);
		array_push(&search_result->files, &new_match);
	}
	
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
	}
	else
	{
		goto failed_to_load_file;
	}
	
	new_result->walking_file_system = false;
	new_result->done_finding_matches = true;
	new_result->done_finding_files = true;
	
	snprintf(global_status_bar.result_status_text, MAX_INPUT_LENGTH, localize("files_matches_comparison"), current_search_result->files_matched, current_search_result->files.length, current_search_result->find_duration_us/1000.0);
	
	array_destroy(&new_result->files);
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
	snprintf(start_path, MAX_INPUT_LENGTH, "%s%s", binary_path, "");
	
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