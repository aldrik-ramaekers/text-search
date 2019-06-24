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
	
	if (path_buf[0] == 0) return 0;
	
	s32 size = ((MAX_INPUT_LENGTH*3) + 53)
		+ matches.length * (matches.entry_size + MAX_INPUT_LENGTH);
	char *buffer = mem_alloc(size);
	memset(buffer, 0, size);
	
	sprintf(buffer, "%s\n%s\n%s\n%.16"PRId64"\n%.1d\n%.1d\n%.8d\n%.8d\n%.8d\n%.1d\n%.1d\n",
			search_result->search_directory_buffer,
			search_result->filter_buffer,
			search_result->text_to_find_buffer,
			search_result->find_duration_us,
			search_result->show_error_message,
			search_result->found_file_matches,
			search_result->files_searched,
			search_result->files_matched,
			search_result->search_result_source_dir_len,
			search_result->match_found,
			*search_result->recursive_state_buffer);
	
	s32 cursor = 0;
	cursor += strlen(buffer);
	
	for (s32 i = 0; i < matches.length; i++)
	{
		text_match* m = array_at(&matches, i);
		
		// path
		char *path = m->file.path;
		while(*path)
		{
			buffer[cursor++] = *path;
			path++;
		}
		buffer[cursor++] = '\n';
		
		// filter
		char *filter = m->file.matched_filter;
		while(*filter)
		{
			buffer[cursor++] = *filter;
			filter++;
		}
		buffer[cursor++] = '\n';
		
		// file error
		char file_error_buf[10];
		char *file_error = file_error_buf;
		
		snprintf(file_error, 10, "%d", m->file_error);
		
		while(*file_error)
		{
			buffer[cursor++] = *file_error;
			file_error++;
		}
		buffer[cursor++] = '\n';
		
		// match count
		char match_count_buf[10];
		char *match_count = match_count_buf;
		
		snprintf(match_count, 10, "%d", m->match_count);
		
		while(*match_count)
		{
			buffer[cursor++] = *match_count;
			match_count++;
		}
		buffer[cursor++] = '\n';
	}
	
	if (!string_contains(path_buf, SEARCH_RESULT_FILE_EXTENSION))
	{
		strcat(path_buf, ".tts");
	}
	
	platform_write_file_content(path_buf, "w", buffer, size);
	mem_free(buffer);
	
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
	if (!string_contains(path_buf, SEARCH_RESULT_FILE_EXTENSION))
	{
		platform_show_message(main_window, localize("invalid_search_result_file"), localize("error_importing_results"));
		return;
	}
	
	platform_destroy_list_file_result(&global_search_result.files);
	scroll_y = 0;
	
	// sprintf(buffer, "%.16lu\n%.1d\n%.1d\n%.8d\n%.8d\n%.8d\n%.1d\n",
	file_content content = platform_read_file_content(path_buf, "r");
	
	if (!content.content || content.file_error)
	{
		platform_destroy_file_content(&content);
	}
	
	char *buffer = content.content;
	char *buffer_start = buffer;
	
	char *search_directory = mem_alloc(MAX_INPUT_LENGTH);
	char *file_filter = mem_alloc(MAX_INPUT_LENGTH);
	char *text_to_find = mem_alloc(MAX_INPUT_LENGTH);
	
	s32 index = 0;
	s32 offset_ = 0;
	// get path, filter and search text
	for (s32 i = 0; i < content.content_length; i++)
	{
		char ch = buffer[i];
		if (ch == '\n')
		{
			buffer[i] = 0;
			
			if (index == 0)
			{
				strncpy(search_directory, buffer+ offset_, MAX_INPUT_LENGTH);
			}
			else if (index == 1)
			{
				strncpy(file_filter, buffer+ offset_, MAX_INPUT_LENGTH);
			}
			else if (index == 2)
			{
				strncpy(text_to_find, buffer+ offset_, MAX_INPUT_LENGTH);
				buffer += i + 1;
				break;
			}
			
			offset_ = i + 1;
			index++;
		}
	}
	
	strncpy(search_result->search_directory_buffer, search_directory, MAX_INPUT_LENGTH);
	strncpy(search_result->filter_buffer, file_filter, MAX_INPUT_LENGTH);
	strncpy(search_result->text_to_find_buffer, text_to_find, MAX_INPUT_LENGTH);
	
	mem_free(search_directory);
	mem_free(file_filter);
	mem_free(text_to_find);
	
	char *find_duration_us; find_duration_us = buffer; buffer[16] = 0; buffer += 17;
	char *show_error_message; show_error_message = buffer; buffer[1] = 0; buffer += 2;
	char *found_file_matches; found_file_matches = buffer; buffer[1] = 0; buffer += 2;
	char *files_searched; files_searched = buffer; buffer[8] = 0; buffer += 9;
	char *files_matched; files_matched = buffer; buffer[8] = 0; buffer += 9;
	char *search_result_source_dir_len; search_result_source_dir_len = buffer; buffer[8] = 0; buffer += 9;
	char *match_found; match_found = buffer; buffer[1] = 0; buffer += 2;
	char *recursive_search; recursive_search = buffer; buffer[1] = 0; buffer += 2;
	
	
	*search_result->recursive_state_buffer = atoi(recursive_search);
	
	search_result->find_duration_us = string_to_u64(find_duration_us);
	search_result->show_error_message = string_to_u8(show_error_message);
	search_result->found_file_matches = string_to_u8(found_file_matches);
	search_result->files_searched = string_to_s32(files_searched);
	search_result->files_matched = string_to_s32(files_matched);
	search_result->search_result_source_dir_len = string_to_s32(search_result_source_dir_len);
	search_result->match_found = string_to_u8(match_found);
	
	s32 offset = buffer_start - buffer;
	s32 expect = 0;
	s32 current_data_start = 0;
	
	text_match match;
	for (s32 i = 0; i < content.content_length-offset; i++)
	{
		char ch = buffer[i];
		
		if (ch == '\n')
		{
			if (expect == 0)
			{
				buffer[i] = 0;
				char *path = mem_alloc(PATH_MAX);
				strncpy(path, buffer+current_data_start, MAX_INPUT_LENGTH);
				
				match.file.path = path;
				
				expect = 1;
				current_data_start = i+1;
			}
			else if (expect == 1)
			{
				buffer[i] = 0;
				char *filter = mem_alloc(PATH_MAX);
				strncpy(filter, buffer+current_data_start, MAX_INPUT_LENGTH);
				
				match.file.matched_filter = filter;
				
				expect = 2;
				current_data_start = i+1;
			}
			else if (expect == 2)
			{
				buffer[i] = 0;
				char file_error[10];
				strncpy(file_error, buffer+current_data_start, 10);
				file_error[9] = 0;
				
				match.file_error = atoi(file_error);
				
				expect = 3;
				current_data_start = i+1;
			}
			else if (expect == 3)
			{
				buffer[i] = 0;
				char match_count[10];
				strncpy(match_count, buffer+current_data_start, 10);
				match_count[9] = 0;
				
				match.match_count = atoi(match_count);
				
				expect = 0;
				current_data_start = i+1;
				
				array_push(&search_result->files, &match);
			}
		}
	}
	
	sprintf(global_status_bar.result_status_text, localize("files_matches_comparison"), global_search_result.files_matched, global_search_result.files.length, global_search_result.find_duration_us/1000.0);
	
	/*
 printf("SEARCH: %s\n", search_directory);
 printf("FILTER: %s\n", file_filter);
 printf("FIND: %s\n", text_to_find);
 
 printf("\nduration: %"PRId64"\n"
  "show error: %d\n"
  "found match: %d\n"
  "files searched: %d\n"
  "files matched: %d\n"
  "len: %d\n"
  "match_found: %d\n",
  search_result->find_duration_us, 
  search_result->show_error_message,
  search_result->found_file_matches,
  search_result->files_searched,
  search_result->files_matched,
  search_result->search_result_source_dir_len,
  search_result->match_found);
*/
	
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
	
	if (path_buf[0] == 0) return 0;
	
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