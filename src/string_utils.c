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