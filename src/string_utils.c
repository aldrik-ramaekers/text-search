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

#define SEARCH_WITHIN_STRING 0

static s32 length_of_word(char *word)
{
	s32 len = 0;
	while(*word)
	{
		if (*word == ' ') return len;
		if (*word == '\n') return len;
		if (*word == 0) return len;
		++len;
		++word;
	}
	return len;
}

static s32 length_of_expr(char *word)
{
	s32 len = 0;
	while(*word)
	{
		if (*word == 0) return len;
		if (*word != '?' && *word != '*')
		{
			++len;
		}
		++word;
	}
	return len;
}

// TODO(Aldrik): were only checking word terminators ' ' and '\n', are there any other end of line characters?
bool string_contains_ex(char *big, char *small, s32 *line_nr, char **line, s32 *word_offset, bool *cancel_search)
{
	bool match_started = false;
	char *small_original = small;
	
	bool save_info = (line_nr != 0);
	
	if (save_info)
	{
		(*line_nr)+=1;
		*word_offset = 0;
		*line = big;
	}
	
	s32 index = 0;
	char *word_start = big;
	
#if !SEARCH_WITHIN_STRING
	bool ignore_word = false;
#endif
	while(*big)
	{
		if (cancel_search && *cancel_search) return false;
		
		char expr_ch = *small;
		char text_ch = *big;
		
		if (text_ch == 0) 
		{
			//printf("--1\n");
			return false;
		}
		
		if (text_ch == '\n' && save_info)
		{
			(*line_nr)+=1;
			*word_offset = 0;
			*line = big;
		}
		else if (save_info)
		{
			(*word_offset)+=1;
		}
		
#if !SEARCH_WITHIN_STRING
		if (text_ch == ' ' || text_ch == '\n')
		{
			if (ignore_word)
				ignore_word = false;
			
			word_start = big;
		}
		
		if (ignore_word)
		{
			big++;
			index++;
			continue;
		}
#endif
		
		if (!match_started)
		{
			if (expr_ch == text_ch) 
			{
#if SEARCH_WITHIN_STRING
				match_started = true;
				small++;
#else
				char text_prev = *(big-1);
				if (index > 0 && ((text_prev != ' ') && (text_prev != '\n'))
					&& text_ch != ' ')
				{
					match_started = false;
					ignore_word = true;
				}
				else
				{
					match_started = true;
					small++;
				}
#endif
			}
			else if (expr_ch == '?')
			{
				match_started = true;
				small++;
			}
			else if (expr_ch == '*') 
			{
				match_started = true;
				//small++;
			}
			if (*small == 0) return true;
		}
		else
		{
			if (expr_ch == '?')
			{
				small++;
			}
			else if (expr_ch == '*')
			{
				small++;
				
				// continue untill next expr char is found in text,
				// match is found if * is last char of expr
				if (*small == 0) return true;
				
#if 1
				s32 chars_left_in_word = length_of_word(big);
				s32 chars_left_in_expr = length_of_expr(small);
				
				expr_ch = *small;
				text_ch = *big;
				while ((expr_ch != text_ch || *(small+1) != *(big+1) ||
						(*(small) == *(big+1) && chars_left_in_word > chars_left_in_expr)))
				{
					big++;
					chars_left_in_word--;
					expr_ch = *small;
					text_ch = *big;
					//printf("[%d] %c %c\n", index, expr_ch, text_ch);
					index++;
					
					if (text_ch == '\n' && save_info)
					{
						(*line_nr)+=1;
						*word_offset = 0;
						*line = big+1;
					}
					else if (save_info)
					{
						(*word_offset)+=1;
					}
					
					if (text_ch == 0) 
					{
						// text and expression have ended, return true. if expression has not ended, return false.
						if (*(small+1) == 0 || *(small+1) == '*')
						{
							return true;
						}
						else
						{
							//printf("--2\n");
							return false;
						}
					}
				}
				
				small++;
				
				if (*small == 0) return true;
				if (text_ch == 0) return true;
#endif
				
			}
			else if (expr_ch != text_ch)
			{
				// match failed
				match_started = false;
				small = small_original;
			}
			else
			{
				small++;
			}
			
			if (*small == 0) 
			{
				if (*(big+1) == 0 || *(big+1) == ' ' || *(big+1) == '\n')
				{
					return true;
				}
				else
				{
					//printf("--3\n");
					return false;
				}
			}
		}
		//printf("%c %c\n", expr_ch, text_ch);
		big++;
		index++;
	}
	
	//printf("--4\n");
	
	if (*(small) == '*' && *(small+1) == 0)
		return true;
	else if (length_of_word(word_start) >= length_of_expr(small_original) && 
			 *(small+1) == '*' && *(small+2) == 0)
		return true;
	else
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
	sprintf(buffer, "%lu", val);
	return buffer;
}

char *s32_to_string(s32 val, char *buffer)
{
	sprintf(buffer, "%d", val);
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