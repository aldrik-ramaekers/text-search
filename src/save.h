/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_SAVE
#define INCLUDE_SAVE

#define SEARCH_RESULT_FILE_EXTENSION "*.json"

bool export_results(search_result *result);
void import_results_from_file(char *path_buf);
void import_results();

#endif