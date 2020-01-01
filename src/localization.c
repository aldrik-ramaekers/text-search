/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

mo_file load_localization_file(u8 *start_addr, u8 *end_addr, u8 *img_start, u8 *img_end,
							   char *locale_id, char *locale_name)
{
	mo_file mo;
	mo.translations = array_create(sizeof(mo_translation));
	
	{
		mo.header = *(mo_header*)start_addr;
		mo.locale_id = mem_alloc(strlen(locale_id)+1);
		string_copyn(mo.locale_id, locale_id, strlen(locale_id)+1);
		
		mo.locale_full = mem_alloc(strlen(locale_name)+1);
		string_copyn(mo.locale_full, locale_name, strlen(locale_name)+1);
		
		mo.icon = assets_load_image(img_start, img_end, false);
		
		char *buffer = (char*)start_addr;
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

char* locale_get_name()
{
	if (!global_localization.active_localization)
	{
		return "[NO LOCALE]";
	}
	
	return global_localization.active_localization->locale_full;
}

char* locale_get_id()
{
	if (!global_localization.active_localization)
	{
		return "[NO LOCALE]";
	}
	
	return global_localization.active_localization->locale_id;
}

bool set_locale(char *country_id)
{
	if (country_id == 0 && global_localization.mo_files.length)
	{
		global_localization.active_localization = array_at(&global_localization.mo_files, 0);
		return true;
	}
	
	for (s32 i = 0; i < global_localization.mo_files.length; i++)
	{
		mo_file *file = array_at(&global_localization.mo_files, i);
		if (strcmp(file->locale_id, country_id) == 0)
		{
			global_localization.active_localization = file;
			return true;
		}
	}
	
	// if localization is not found, default to first in list (english), return false to report error
	if (global_localization.mo_files.length)
		global_localization.active_localization = array_at(&global_localization.mo_files, 0);
	
	global_localization.active_localization = 0;
	return false;
}

char* localize(const char *identifier)
{
	if (!global_localization.active_localization)
	{
		//printf("NO LOCALE SELECTED.");
		return (char*)identifier;
	}
	
	s32 len = strlen(identifier);
	for (s32 i = 0; i < global_localization.active_localization->translations.length; i++)
	{
		mo_translation *trans = array_at(&global_localization.active_localization->translations, i);
		
		if (trans->identifier_len == len && strcmp(identifier, trans->identifier) == 0)
		{
			return trans->translation;
		}
	}
	printf("MISSING TRANSLATION: [%s][%s]\n", identifier, global_localization.active_localization->locale_id);
	return "MISSING";
}

void load_available_localizations()
{
	global_localization.mo_files = array_create(sizeof(mo_file));
	array_reserve(&global_localization.mo_files, 10);
	
	mo_file en = load_localization_file(_binary____data_translations_en_English_mo_start,
										_binary____data_translations_en_English_mo_end,
										_binary____data_imgs_en_png_start,
										_binary____data_imgs_en_png_end,
										"en", "English");
	
	mo_file nl = load_localization_file(_binary____data_translations_nl_Dutch_mo_start,
										_binary____data_translations_nl_Dutch_mo_end,
										_binary____data_imgs_nl_png_start,
										_binary____data_imgs_nl_png_end,
										"nl", "Dutch");
	
	array_push(&global_localization.mo_files, &en);
	array_push(&global_localization.mo_files, &nl);
}

void destroy_available_localizations()
{
	for (s32 i = 0; i < global_localization.mo_files.length; i++)
	{
		mo_file *file = array_at(&global_localization.mo_files, i);
		array_destroy(&file->translations);
		mem_free(file->locale_id);
		mem_free(file->locale_full);
		
		if (file->icon)
			assets_destroy_image(file->icon);
	}
	array_destroy(&global_localization.mo_files);
}
