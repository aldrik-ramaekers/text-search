#ifndef INCLUDE_LOCALIZATION
#define INCLUDE_LOCALIZATION

// https://www.science.co.il/language/Locale-codes.php

typedef struct t_mo_entry
{
	s32 length;
	s32 offset;
} mo_entry;

typedef struct t_mo_translation
{
	s32 identifier_len;
	char *identifier;
	char *translation;
} mo_translation;

typedef struct t_mo_header
{
	s32 magic_number;
	s32 file_format_revision;
	s32 number_of_strings;
	s32 identifier_table_offset;
	s32 translation_table_offset;
	s32 hashtable_size;
	s32 hashtable_offset;
} mo_header;

typedef struct t_mo_file
{
	mo_header header;
	array translations;
	char *locale_id;
	char *locale_full;
	image *icon;
	file_content content;
} mo_file;

typedef struct t_localization
{
	array mo_files;
	mo_file *active_localization;
} localization;

localization global_localization;

char* localize_get_id();
char* localize_get_name();
char* localize(const char *identifier);
void set_locale(char *country_id);
void load_available_localizations();

#endif