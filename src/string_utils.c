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

#define SEARCH_WITHIN_STRING 1

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

bool string_contains(char *big, char *small)
{
	bool match_started = false;
	char *small_original = small;
	
	s32 index = 0;
	char *word_start = big;
#if !SEARCH_WITHIN_STRING
	bool ignore_word = false;
#endif
	while(*big)
	{
		char expr_ch = *small;
		char text_ch = *big;
		
		if (text_ch == 0) 
		{
			//printf("--1\n");
			return false;
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
				if ((index > 0 && text_prev != ' ') && text_ch != ' ')
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