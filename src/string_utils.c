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

bool string_match(char *first, char *second)
{
	// If we reach at the end of both strings, we are done 
    if (*first == '\0' && *second == '\0') 
        return true; 
	
    // Make sure that the characters after '*' are present 
    // in second string. This function assumes that the first 
    // string will not contain two consecutive '*' 
    if (*first == '*' && *(first+1) != '\0' && *second == '\0') 
        return false;
	
    // If the first string contains '?', or current characters 
    // of both strings string_match
    if (*first == '?' || *first == *second) 
        return string_match(first+1, second+1);
	
    // If there is *, then there are two possibilities 
    // a) We consider current character of second string 
    // b) We ignore current character of second string. 
    if (*first == '*') 
        return string_match(first+1, second) || string_match(first, second+1); 
    return false; 
}

bool string_contains_ex(char *text_to_search, char *text_to_find, array *text_matches, bool *cancel_search)
{
	bool final_result = false;
	
	// * wildcard at the start of text to find is not needed
	if (*text_to_find == '*') text_to_find++;
	
	char *text_to_find_original = text_to_find;
	bool save_info = (text_matches != 0);
	
	utf8_int32_t text_to_search_ch = 0;
	utf8_int32_t text_to_find_ch = 0;
	size_t text_to_find_char_len = utf8len(text_to_find);
	
	s32 line_nr_val = 1;
	s32 word_offset_val = 0;
	s32 word_match_len_val = 0;
	char* line_start_ptr = text_to_search;
	
	s32 index = 0;
	while((text_to_search = utf8codepoint(text_to_search, &text_to_search_ch)) 
		  && text_to_search_ch)
	{
		if (cancel_search && *cancel_search) goto set_info_and_return_failure;
		
		word_offset_val++;
		if (text_to_search_ch == '\n') 
		{
			line_nr_val++;
			word_offset_val = 0;
			line_start_ptr = text_to_search;
		}
		
		char *text_to_search_current_attempt = text_to_search;
		utf8_int32_t text_to_search_current_attempt_ch = 0;
		
		bool in_wildcard = false;
		
		text_to_find = utf8codepoint(text_to_find, &text_to_find_ch);
		text_to_search_current_attempt = utf8codepoint(text_to_search_current_attempt,
													   &text_to_search_current_attempt_ch);
		
		word_match_len_val = 0;
		while(text_to_search_current_attempt_ch)
		{
			if (cancel_search && *cancel_search) goto set_info_and_return_failure;
			
			// wildcard, accept any character in text to search
			if (text_to_find_ch == '?')
				goto continue_search;
			
			// character matches, 
			if (text_to_find_ch == text_to_search_current_attempt_ch && in_wildcard)
				in_wildcard = false;
			
			// wildcard, accept any characters in text to search untill next char is found
			if (text_to_find_ch == '*')
			{
				text_to_find = utf8codepoint(text_to_find, &text_to_find_ch);
				in_wildcard = true;
			}
			
			// text to find has reached 0byte, word has been found
			if (text_to_find_ch == 0)
			{
				if (save_info)
				{
					text_match new_match;
					new_match.line_nr = line_nr_val;
					new_match.word_offset = word_offset_val;
					new_match.word_match_len = word_match_len_val;
					new_match.line_start = line_start_ptr;
					new_match.line_info = 0;
					array_push(text_matches, &new_match);
				}
				
				final_result = true;
				break;
			}
			
			// character does not match, continue search
			if (text_to_find_ch != text_to_search_current_attempt_ch && !in_wildcard)
				break;
			
			continue_search:
			if (!in_wildcard)
				text_to_find = utf8codepoint(text_to_find, &text_to_find_ch);
			
			text_to_search_current_attempt = utf8codepoint(
				text_to_search_current_attempt,
				&text_to_search_current_attempt_ch);
			
			word_match_len_val++;
		}
		
		text_to_find = text_to_find_original;
		index++;
	}
	
	return final_result;
	
	set_info_and_return_failure:
	return false;
}

static char *ltrim(char *str, const char *seps)
{
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn(str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove(str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}

static char *rtrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

inline void string_trim(char *string)
{
	ltrim(rtrim(string, 0), 0);
}

inline bool string_equals(char *first, char *second)
{
	return (strcmp(first, second) == 0);
}

char *u64_to_string(u64 val, char *buffer)
{
#ifdef OS_LINUX
	sprintf(buffer, "%lu", val);
#endif
#ifdef IS_WIN
	sprintf(buffer, "%I64u", val);
#endif
	return buffer;
}

char *s32_to_string(s32 val, char *buffer)
{
#ifdef OS_LINUX
	sprintf(buffer, "%d", val);
#endif
#ifdef IS_WIN
	sprintf(buffer, "%I32s", val);
#endif
	return buffer;
}

// replaces " with \" for file formats
void string_appendf(char *buffer, char *text)
{
	u32 len = strlen(buffer);
	while(*text)
	{
		if (*text < 32)
		{
			buffer[len] = ' ';
			len++;
			text++;
			continue;
		}
		
		if (*text == '"')
		{
			buffer[len] = '\\';
			len++;
		}
		if (*text == '\\')
		{
			buffer[len] = '\\';
			len++;
		}
		
		buffer[len] = *text;
		len++;
		text++;
	}
}

void string_append(char *buffer, char *text)
{
	u32 len = strlen(buffer);
	while(*text)
	{
		buffer[len] = *text;
		len++;
		text++;
	}
}

bool string_remove(char **buffer, char *text)
{
	s32 len = strlen(text);
	char tmp[200];
	memcpy(tmp, *buffer, len);
	memset(tmp+len, 0, 1);
	
	if (string_equals(tmp, text))
	{
		*buffer += len;
		return true;
	}
	
	return false;
}

char* string_get_json_literal(char **buffer, char *tmp)
{
	char *buf_start = *buffer;
	char *buf = *buffer;
	s32 len = 0;
	while(*buf)
	{
		if ((*buf == ',' || *buf == '}') && (len > 0 && *(buf-1) == '"') && (len > 1 && *(buf-2) != '\\'))
		{
			memcpy(tmp, buf_start, len);
			memset(tmp+len-1, 0, 1);
			*buffer += len-1;
			return tmp;
		}
		
		len++;
		buf++;
	}
	
	return tmp;
}

s32 string_get_json_ulong_number(char **buffer)
{
	char tmp[20];
	char *buf_start = *buffer;
	char *buf = *buffer;
	s32 len = 0;
	while(*buf)
	{
		if (*buf == ',' || *buf == '}')
		{
			memcpy(tmp, buf_start, len);
			memset(tmp+len, 0, 1);
			*buffer += len;
			return string_to_u64(tmp);
		}
		
		len++;
		buf++;
	}
	
	return 0;
}

s32 string_get_json_number(char **buffer)
{
	char tmp[20];
	char *buf_start = *buffer;
	char *buf = *buffer;
	s32 len = 0;
	while(*buf)
	{
		if (*buf == ',' || *buf == '}')
		{
			memcpy(tmp, buf_start, len);
			memset(tmp+len, 0, 1);
			*buffer += len;
			return string_to_s32(tmp);
		}
		
		len++;
		buf++;
	}
	
	return 0;
}

void utf8_str_remove_range(char *str, s32 from, s32 to)
{
	char *orig_str = str;
	s32 i = 0;
	utf8_int32_t ch = 0;
	s32 total_len = strlen(str)+1+4;
	char *replacement = calloc(total_len,1);
	char *rep_off = replacement;
	replacement[0] = 0;
	
	while((str = utf8codepoint(str, &ch)) && ch)
	{
		if (i < from || i >= to)
		{
			rep_off = utf8catcodepoint(rep_off, ch, 5);
		}
		
		++i;
	}
	*rep_off = 0;
	
	strcpy(orig_str, replacement);
}

void utf8_str_remove_at(char *str, s32 at)
{
	char *orig_str = str;
	s32 i = 0;
	utf8_int32_t ch = 0;
	s32 total_len = strlen(str)+1+4;
	char *replacement = calloc(total_len,1);
	char *rep_off = replacement;
	replacement[0] = 0;
	
	while((str = utf8codepoint(str, &ch)) && ch)
	{
		if (at != i)
		{
			rep_off = utf8catcodepoint(rep_off, ch, 5);
		}
		
		++i;
	}
	*rep_off = 0;
	
	strcpy(orig_str, replacement);
}

void utf8_str_insert_utf8str(char *str, s32 at, char *toinsert)
{
	s32 index = 0;
	utf8_int32_t ch;
	while((toinsert = utf8codepoint(toinsert, &ch)) && ch)
	{
		utf8_str_insert_at(str, at+index, ch);
		index++;
	}
}

void utf8_str_insert_at(char *str, s32 at, utf8_int32_t newval)
{
	char *orig_str = str;
	s32 i = 0;
	utf8_int32_t ch = 0;
	s32 total_len = strlen(str)+1+4;
	char *replacement = calloc(total_len,1);
	char *rep_off = replacement;
	replacement[0] = 0;
	
	while((str = utf8codepoint(str, &ch)))
	{
		if (at == i)
		{
			rep_off = utf8catcodepoint(rep_off, newval, 5);
		}
		
		rep_off = utf8catcodepoint(rep_off, ch, 5);
		
		++i;
		
		if (!ch) break;
	}
	*rep_off = 0;
	
	strcpy(orig_str, replacement);
}

char *utf8_str_copy_upto(char *str, s32 roof, char *buffer)
{
	utf8_int32_t ch = 0;
	s32 index = 0;
	char *orig_buffer = buffer;
	while((str = utf8codepoint(str, &ch)) && ch)
	{
		if (index == roof) break;
		buffer = utf8catcodepoint(buffer, ch, 5);
		index++;
	}
	buffer = utf8catcodepoint(buffer, 0, 5);
	
	return orig_buffer;
}

char *utf8_str_copy_range(char *str, s32 floor, s32 roof, char *buffer)
{
	utf8_int32_t ch = 0;
	s32 index = 0;
	char *orig_buffer = buffer;
	while((str = utf8codepoint(str, &ch)) && ch)
	{
		if (index == roof) break;
		if (index >= floor)
			buffer = utf8catcodepoint(buffer, ch, 5);
		index++;
	}
	buffer = utf8catcodepoint(buffer, 0, 5);
	
	return orig_buffer;
}

void utf8_str_replace_at(char *str, s32 at, utf8_int32_t newval)
{
	char *orig_str = str;
	s32 i = 0;
	utf8_int32_t ch = 0;
	s32 total_len = strlen(str)+1+4;
	char *replacement = calloc(total_len,1);
	char *rep_off = replacement;
	replacement[0] = 0;
	
	while((str = utf8codepoint(str, &ch)) && ch)
	{
		if (at == i)
		{
			rep_off = utf8catcodepoint(rep_off, newval, 5);
		}
		else
		{
			rep_off = utf8catcodepoint(rep_off, ch, 5);
		}
		++i;
	}
	*rep_off = 0;
	
	strcpy(orig_str, replacement);
}

char* utf8_str_upto(char *str, s32 index)
{
	s32 i = 0;
	utf8_int32_t ch;
	char *prev_str = str;
	while((str = utf8codepoint(str, &ch)) && ch)
	{
		if (index == i) return prev_str;
		prev_str = str;
		++i;
	}
	
	return str;
}

utf8_int32_t utf8_str_at(char *str, s32 index)
{
	s32 i = 0;
	utf8_int32_t ch;
	while((str = utf8codepoint(str, &ch)) && ch)
	{
		if (index == i) return ch;
		
		++i;
	}
	
	return 0;
}