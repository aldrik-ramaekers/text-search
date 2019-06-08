
static void *export_result_d(void *arg)
{
	search_result *search_result = arg;
	
	array matches = search_result->files;
	
	char path_buf[MAX_INPUT_LENGTH];
	path_buf[0] = 0;
	
	struct open_dialog_args *args = malloc(sizeof(struct open_dialog_args));
	args->buffer = path_buf;
	args->type = SAVE_FILE;
	
	platform_open_file_dialog_d(args);
	
	if (path_buf[0] == 0) return 0;
	if (path_buf[1] == 0) return 0;
	
	s32 size = matches.length * (matches.entry_size + MAX_INPUT_LENGTH);
	char *buffer = malloc(size);
	memset(buffer, 0, size);
	
	sprintf(buffer, "%.16lu\n%.1d\n%.1d\n%.8d\n%.8d\n%.8d\n%.1d\n",
			search_result->find_duration_us,
			search_result->show_error_message,
			search_result->found_file_matches,
			search_result->files_searched,
			search_result->files_matched,
			search_result->search_result_source_dir_len,
			search_result->match_found);
	
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
	
	platform_write_file_content(path_buf, "w", buffer, size);
	free(buffer);
	
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

static void* import_results_d(void *arg)
{
	for (s32 i = 0; i < global_search_result.files.length; i++)
	{
		text_match *match = array_at(&global_search_result.files, i);
		free(match->file.path);
		free(match->file.matched_filter);
	}
	global_search_result.files.length = 0;
	scroll_y = 0;
	
	search_result *search_result = arg;
	
	array matches = search_result->files;
	
	char path_buf[MAX_INPUT_LENGTH];
	struct open_dialog_args *args = malloc(sizeof(struct open_dialog_args));
	args->buffer = path_buf;
	args->type = OPEN_FILE;
	
	platform_open_file_dialog_d(args);
	
	if (path_buf[0] == 0) return 0;
	if (path_buf[1] == 0) return 0;
	
	// sprintf(buffer, "%.16lu\n%.1d\n%.1d\n%.8d\n%.8d\n%.8d\n%.1d\n",
	file_content content = platform_read_file_content(path_buf, "r");
	
	if (!content.content || content.file_error)
	{
		platform_destroy_file_content(&content);
		return 0;
	}
	
	char *buffer = content.content;
	char *buffer_start = buffer;
	
	char *find_duration_us; find_duration_us = buffer; buffer[16] = 0; buffer += 17;
	char *show_error_message; show_error_message = buffer; buffer[1] = 0; buffer += 2;
	char *found_file_matches; found_file_matches = buffer; buffer[1] = 0; buffer += 2;
	char *files_searched; files_searched = buffer; buffer[8] = 0; buffer += 9;
	char *files_matched; files_matched = buffer; buffer[8] = 0; buffer += 9;
	char *search_result_source_dir_len; search_result_source_dir_len = buffer; buffer[8] = 0; buffer += 9;
	char *match_found; match_found = buffer; buffer[1] = 0; buffer += 2;
	
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
				char *path = malloc(PATH_MAX);
				strcpy(path, buffer+current_data_start);
				
				match.file.path = path;
				
				expect = 1;
				current_data_start = i+1;
			}
			else if (expect == 1)
			{
				buffer[i] = 0;
				char *filter = malloc(PATH_MAX);
				strcpy(filter, buffer+current_data_start);
				
				match.file.matched_filter = filter;
				
				expect = 2;
				current_data_start = i+1;
			}
			else if (expect == 2)
			{
				buffer[i] = 0;
				char file_error[10];
				strcpy(file_error, buffer+current_data_start);
				
				match.file_error = atoi(file_error);
				
				expect = 3;
				current_data_start = i+1;
			}
			else if (expect == 3)
			{
				buffer[i] = 0;
				char match_count[10];
				strcpy(match_count, buffer+current_data_start);
				
				match.match_count = atoi(match_count);
				
				expect = 0;
				current_data_start = i+1;
				
				array_push(&search_result->files, &match);
			}
		}
	}
	
	sprintf(global_status_bar.result_status_text, "%d out of %d files matched in %.2fms", global_search_result.files_matched, global_search_result.files.length, global_search_result.find_duration_us/1000.0);
	
#if 0
	printf("\nduration: %lu\n"
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
#endif
	
	platform_destroy_file_content(&content);
	
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
