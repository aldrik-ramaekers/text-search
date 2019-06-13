mo_file load_localization_file(char *path)
{
	mo_file mo;
	mo.translations = array_create(sizeof(mo_translation));
	
	set_active_directory(binary_path);
	file_content content = platform_read_file_content(path, "rb");
	mo.content = content;
	
	if (content.content && !content.file_error && content.content_length > sizeof(mo_header))
	{
		mo.header = *(mo_header*)content.content;
		
		// get country id
		s32 len = strlen(path);
		mo.locale = mem_alloc(len);
		strcpy(mo.locale, path+len-5);
		mo.locale[2] = 0;
		
#if 0
		printf("magic: %d\nfile revision: %d\nstring count: %d\nidentifier offset: %d\ntranslation offset: %d\nhashtable size: %d\nhashtable offset: %d\n", mo.header.magic_number, mo.header.file_format_revision, mo.header.number_of_strings, mo.header.identifier_table_offset, mo.header.translation_table_offset, mo.header.hashtable_size, mo.header.hashtable_offset);
#endif
		
		char *buffer = content.content;
		mo_entry *identifiers = (mo_entry*)(buffer + mo.header.identifier_table_offset);
		mo_entry *translations = (mo_entry*)(buffer + mo.header.translation_table_offset);
		
		// skip first one because this is file information
		for (s32 i = 1; i < mo.header.number_of_strings; i++)
		{
			mo_entry *entry = &identifiers[i];
			mo_entry *trans = &translations[i];
			
			mo_translation translation;
			translation.identifier_len = entry->length;
			translation.identifier = buffer+entry->offset;
			translation.translation = buffer+trans->offset;
			
			array_push(&mo.translations, &translation);
			//printf("%s=%s\n", translation.identifier, translation.translation);
		}
	}
	
	
	return mo;
}

void set_locale(char *country_id)
{
	for (s32 i = 0; i < global_localization.mo_files.length; i++)
	{
		mo_file *file = array_at(&global_localization.mo_files, i);
		if (strcmp(file->locale, country_id) == 0)
		{
			global_localization.active_localization = file;
			return;
		}
	}
	
	global_localization.active_localization = 0;
	return;
}

char* localize(const char *identifier)
{
	s32 len = strlen(identifier);
	for (s32 i = 0; i < global_localization.active_localization->translations.length; i++)
	{
		mo_translation *trans = array_at(&global_localization.active_localization->translations, i);
		
		if (trans->identifier_len == len && strcmp(identifier, trans->identifier) == 0)
		{
			return trans->translation;
		}
	}
	
	return "MISSING";
}

void load_available_localizations()
{
	global_localization.mo_files = array_create(sizeof(mo_file));
	array_reserve(&global_localization.mo_files, 10);
	
	array file_list = array_create(sizeof(found_file));
	
	set_active_directory(binary_path);
	platform_list_files_block(&file_list, "data/translations/", 
							  "*.mo", false);
	
	for (s32 i = 0; i < file_list.length; i++)
	{
		found_file *file = array_at(&file_list, i);
		mo_file mo = load_localization_file(file->path);
		s32 index = array_push(&global_localization.mo_files, &mo);
		mem_free(file->path);
		mem_free(file->matched_filter);
	}
	
	array_destroy(&file_list);
}

void destroy_available_localizations()
{
	for (s32 i = 0; i < global_localization.mo_files.length; i++)
	{
		mo_file *file = array_at(&global_localization.mo_files, i);
		array_destroy(&file->translations);
		mem_free(file->locale);
		platform_destroy_file_content(&file->content);
	}
}
