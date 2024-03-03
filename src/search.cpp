#include "search.h"
#include "platform.h"

array get_filters(char *pattern)
{
	array result = array_create(MAX_INPUT_LENGTH);
	
	char current_filter[MAX_INPUT_LENGTH];
	int filter_len = 0;
	while(*pattern)
	{
		char ch = *pattern;
		
		if (ch == ',')
		{
			current_filter[filter_len] = 0;
			array_push(&result, current_filter);
			filter_len = 0;
		}
		else
		{
			if(filter_len < MAX_INPUT_LENGTH-1)
			{
				current_filter[filter_len++] = ch;
			}
			else
			{
				current_filter[filter_len] = ch;
			}
		}
		
		pattern++;
	}
	current_filter[filter_len] = 0;
	array_push(&result, current_filter);
	
	return result;
}

int string_match(char *first, char *second)
{
	// If we reach at the end of both strings, we are done 
	if (*first == '\0' && *second == '\0') 
		return 1; 
	
	// Make sure that the characters after '*' are present 
	// in second string. This function assumes that the first 
	// string will not contain two consecutive '*' 
	if (*first == '*' && *(first+1) != '\0' && *second == '\0') 
		return 0;
	
	// If the first string contains '?', or current characters 
	// of both strings string_match
	if (*first == '?' || *first == *second) 
		return string_match(first+1, second+1);
	
	// If there is *, then there are two possibilities 
	// a) We consider current character of second string 
	// b) We ignore current character of second string. 
	if (*first == '*') 
		return string_match(first+1, second) || string_match(first, second+1); 
	return 0; 
}


int filter_matches(array *filters, char *string, char **matched_filter)
{
	for (int i = 0; i < filters->length; i++)
	{
		char *filter = (char *)array_at(filters, i);
		if (string_match(filter, string))
		{
			*matched_filter = filter;
			return strlen(filter);
		}
	}
	return -1;
}

search_result *create_empty_search_result()
{
	search_result *new_result_buffer = (search_result *)malloc(sizeof(search_result));
	new_result_buffer->completed_match_threads = 0;
	new_result_buffer->mutex = mutex_create();
	new_result_buffer->done_finding_files = false;
	new_result_buffer->file_list_read_cursor = 0;
	new_result_buffer->max_thread_count = 4;
	new_result_buffer->match_count = 0;
	new_result_buffer->file_count = 0;
	new_result_buffer->max_file_size = megabytes(1000);
	
	new_result_buffer->files = array_create(sizeof(found_file));
	new_result_buffer->files.reserve_jump = FILE_RESERVE_COUNT;
	array_reserve(&new_result_buffer->files, FILE_RESERVE_COUNT);

	new_result_buffer->matches = array_create(sizeof(file_match));
	new_result_buffer->matches.reserve_jump = FILE_RESERVE_COUNT;
	array_reserve(&new_result_buffer->matches, FILE_RESERVE_COUNT);

	// filter buffers
	new_result_buffer->directory_to_search = (char*)malloc(MAX_INPUT_LENGTH);
	new_result_buffer->search_text = (char*)malloc(MAX_INPUT_LENGTH);
	
	return new_result_buffer;
}

bool string_is_asteriks(char *text)
{
	utf8_int32_t ch;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch != '*') return false;
	}
	return true;
}

bool string_contains_ex(char *text_to_search, utf8_int8_t *text_to_find, array *text_matches)
{
	bool final_result = false;
	bool is_asteriks_only = false;
	
	// * wildcard at the start of text to find is not needed
	if (string_is_asteriks(text_to_find))
	{
		is_asteriks_only = true;
		text_to_find += strlen(text_to_find);
	}
	
	// remove all asteriks from start
	utf8_int32_t br;
	while(utf8codepoint(text_to_find, &br) && br == '*')
	{
		text_to_find = utf8codepoint(text_to_find, &br);
	}
	
	char *text_to_find_original = text_to_find;
	bool save_info = (text_matches != 0);
	
	utf8_int32_t text_to_search_ch = 0;
	utf8_int32_t text_to_find_ch = 0;
	size_t text_to_find_char_len = utf8len(text_to_find);
	
	int line_nr_val = 1;
	int word_offset_val = 0;
	int word_match_len_val = 0;
	char* line_start_ptr = text_to_search;
	
	int index = 0;
	while((text_to_search = utf8codepoint(text_to_search, &text_to_search_ch)) 
		  && text_to_search_ch)
	{
		word_offset_val++;
		if (text_to_search_ch == '\n') 
		{
			line_nr_val++;
			word_offset_val = 0;
			line_start_ptr = text_to_search;
		}
		
		utf8_int8_t *text_to_search_current_attempt = text_to_search;
		utf8_int32_t text_to_search_current_attempt_ch = text_to_search_ch;
		
		bool in_wildcard = false;
		
		text_to_find = utf8codepoint(text_to_find, &text_to_find_ch);
		//text_to_search_current_attempt = utf8codepoint(text_to_search_current_attempt,
		//&text_to_search_current_attempt_ch);
		
		word_match_len_val = 0;
		while(text_to_search_current_attempt_ch)
		{
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
				done:
				if (save_info)
				{
					text_match new_match;
					new_match.line_nr = line_nr_val;
					new_match.word_offset = word_offset_val-1;
					new_match.word_match_len = word_match_len_val;
					new_match.line_start = line_start_ptr;
					new_match.line_info = 0;
					array_push(text_matches, &new_match);
				}
				
				final_result = true;
				
				if (is_asteriks_only)
				{
					return final_result;
				}
				
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
			
			if (!text_to_search_current_attempt_ch && !text_to_find_ch) goto done;
			
			word_match_len_val++;
		}
		
		text_to_find = text_to_find_original;
		index++;
	}
	
	return final_result;
	
	set_info_and_return_failure:
	return false;
}