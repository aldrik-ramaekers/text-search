
bool export_results(array matches)
{
	bool result = false;
	
	s32 size = matches.length * matches.entry_size * MAX_INPUT_LENGTH;
	char *buffer = malloc(size);
	memset(buffer, 0, size);
	
	s32 cursor = 0;
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
	snprintf(save_file, MAX_INPUT_LENGTH, "%s%s", binary_path, "/data/export/1.txt");
	result = platform_write_file_content(save_file, "w", buffer, size);
	
	free(save_file);
	free(buffer);
	return result;
}

array import_results()
{
	array new_array = array_create(sizeof(text_match));
	
	return new_array;
}
