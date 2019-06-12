
settings_config settings_config_load_from_file(char *path)
{
	settings_config config;
	config.settings = array_create(sizeof(config_setting));
	
	set_active_directory(binary_path);
	
	file_content content = platform_read_file_content(path, "r");
	
	if (!content.content || content.file_error)
	{
		platform_destroy_file_content(&content);
		return config;
	}
	
	config_setting current_entry;
	current_entry.name = 0;
	current_entry.value = 0;
	
	s32 token_offset = 0;
	bool in_literal = false;
	for (s32 i = 0; i < content.content_length; i++)
	{
		char ch = ((char*)content.content)[i];
		
		if (ch == '=')
		{
			((char*)content.content)[i] = 0;
			current_entry.name = mem_alloc((i - token_offset)+1);
			strcpy(current_entry.name, content.content+token_offset);
			string_trim(current_entry.name);
		}
		if (ch == '"')
		{
			in_literal = !in_literal;
			
			if (!in_literal)
			{
				((char*)content.content)[i] = 0;
				current_entry.value = mem_alloc((i - token_offset)+1);
				strcpy(current_entry.value, content.content+token_offset);
			}
			
			token_offset = i+1;
		}
		
		if (ch == '\n')
		{
			token_offset = i+1;
			array_push(&config.settings, &current_entry);
		}
	}
	array_push(&config.settings, &current_entry);
	
	platform_destroy_file_content(&content);
	
	return config;
}

void settings_config_destroy(settings_config *config)
{
	for (s32 i = 0; i < config->settings.length; i++)
	{
		config_setting *entry = array_at(&config->settings, i);
		printf("SETTING: %s=%s\n", entry->name, entry->value);
		
		mem_free(entry->name);
		mem_free(entry->value);
	}
	
	array_destroy(&config->settings);
}