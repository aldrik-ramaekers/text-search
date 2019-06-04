
bool export_results(search_result *search_result)
{
	array matches = search_result->files;
	
	bool result = false;
	
	s32 size = matches.length * matches.entry_size * MAX_INPUT_LENGTH;
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
	
	char *save_file = malloc(MAX_INPUT_LENGTH);
	
	u64 stamp = platform_get_time(TIME_FULL, TIME_MS);
	snprintf(save_file, MAX_INPUT_LENGTH, "%s%s%lu%s", binary_path, "/data/export/", stamp, ".tss");
	result = platform_write_file_content(save_file, "w", buffer, size);
	
	free(save_file);
	free(buffer);
	return result;
}

search_result import_results()
{
	search_result result;
#if 0
	result.find_duration_us,
	result.show_error_message,
	result.found_file_matches,
	result.files_searched,
	result.files_matched,
	result.search_result_source_dir_len,
	result.match_found
#endif
		return result;
}
