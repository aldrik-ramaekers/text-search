/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_STRING_UTILS
#define INCLUDE_STRING_UTILS

#if 0
printf("[lll][llll*] : %d\n", string_contains("lll", "llll*"));
printf("[lllll][l*lop] : %d\n", string_contains("lllll", "l*lop"));
printf("[22lllll][l*l] : %d\n", string_contains("22lllll", "l*l"));

printf("[hello world][h?lo] : %d\n", string_contains("hello world", "h?lo"));
printf("[\"hello sailor\"][sailor] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "sailor"));
printf("[\"hello sailor\"][*sailor] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "*sailor"));
printf("\n");

printf("[\"hello sailor\"][*sailor\"] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "*sailor\""));
printf("[\"hello sailor\"][*sailor*] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "*sailor*"));
printf("[\"hello sailor\"][sailor*] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "sailor*"));
printf("[22lllll pi23hjp rbksje LSKJDh l][LS*] : %d\n",
	   string_contains("22lllll pi23hjp rbksje LSKJDh l", "LS*"));
printf("[22lllll lal][l*l] : %d\n", string_contains("22lllll lal", "l*l"));
printf("[22lllll][*l*l] : %d\n", string_contains("lllll", "*l*l"));
printf("[hello world][hello] : %d\n", string_contains("hello world", "hello"));
printf("[hello world][h?llo] : %d\n", string_contains("hello world", "h?llo"));
printf("[hello world][h????] : %d\n", string_contains("hello world", "h????"));
printf("[hello world][h*lo] : %d\n", string_contains("hello world", "h*lo"));
printf("[hello world][*] : %d\n", string_contains("hello world", "*"));
printf("[hello world][h*] : %d\n", string_contains("hello world", "h*"));
printf("[hello world][*o] : %d\n", string_contains("hello world", "*o"));
printf("[hello world][h*o] : %d\n", string_contains("hello world", "h*o"));
printf("[hello world][*lo] : %d\n", string_contains("hello world", "*lo"));
printf("[hello world][hel*lo] : %d\n", string_contains("hello world", "hel*lo"));

printf("[lllll][l*l] : %d\n", string_contains("lllll", "l*l"));
printf("[llllllll][l*llll] : %d\n", string_contains("lllll", "l*llll"));
printf("[llllllll][l*lll] : %d\n", string_contains("lllll", "l*lll"));
printf("[llllllll][llll*l] : %d\n", string_contains("lllll", "llll*l"));
printf("[llllllll][*] : %d\n", string_contains("lllll", "*"));
printf("[lllll][l?lll] : %d\n", string_contains("lllll", "l?lll"));

printf("[lllll][lllll] : %d\n", string_contains("lllll", "lllll"));
printf("[lllll][*llll] : %d\n", string_contains("lllll", "*llll"));
printf("[lllll][llll*] : %d\n", string_contains("lllll", "llll*"));
printf("[lllll][*llll*] : %d\n", string_contains("lllll", "*llll*"));
printf("[lllll][*lllll*] : %d\n", string_contains("lllll", "*lllll*"));
printf("[lllll][*ll*] : %d\n", string_contains("lllll", "*ll*"));
#endif

typedef struct t_text_match
{
	u32 line_nr;
	s32 word_offset;
	s32 word_match_len;
	char *line_start;
	char *line_info;
} text_match;

#define string_contains(big, small) string_contains_ex(big, small, 0, 0)
bool string_match(char *first, char *second);
bool string_contains_ex(char *big, char *small, array *text_matches, bool *cancel_search);
void string_trim(char *string);
bool string_equals(char *first, char *second);
void string_append(char *buffer, char *text);
void string_copyn(char *buffer, char *text, s32 bufferlen);
void string_appendn(char *buffer, char *text, s32 bufferlen);
void string_appendf(char *buffer, char *text);
bool string_remove(char **buffer, char *text);
char* string_get_json_literal(char **buffer, char *tmp);
s32 string_get_json_number(char **buffer);
s32 string_get_json_ulong_number(char **buffer);

utf8_int32_t utf8_str_at(char *str, s32 index);
void utf8_str_remove_at(char *str, s32 at);
void utf8_str_remove_range(char *str, s32 from, s32 to);
void utf8_str_insert_at(char *str, s32 at, utf8_int32_t newval);
void utf8_str_insert_utf8str(char *str, s32 at, char *toinsert);
void utf8_str_replace_at(char *str, s32 at, utf8_int32_t newval);
char* utf8_str_upto(char *str, s32 index);
char *utf8_str_copy_upto(char *str, s32 roof, char *buffer);
char *utf8_str_copy_range(char *str, s32 floor, s32 roof, char *buffer);

#endif