/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

void find_text_in_files(search_result *search_result);
s32 prepare_search_directory_path(char *path, s32 len);
search_result *create_empty_search_result();
void do_search();
bool export_results(search_result *result);

static void print_license_message()
{
	time_t t = time(0);
	struct tm *now = localtime(&t);
	
	printf("BSD 2-Clause License\n"
		   "\n"
		   "Copyright (c) %d, Aldrik Ramaekers\n"
		   "All rights reserved.\n"
		   "\n"
		   "Redistribution and use in source and binary forms, with or without\n"
		   "modification, are permitted provided that the following conditions are met:\n"
		   "\n"
		   "1. Redistributions of source code must retain the above copyright notice, this\n"
		   "list of conditions and the following disclaimer.\n"
		   "\n"
		   "2. Redistributions in binary form must reproduce the above copyright notice,\n"
		   "this list of conditions and the following disclaimer in the documentation\n"
		   "and/or other materials provided with the distribution.\n"
		   "\n"
		   "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
		   "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
		   "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
		   "DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
		   "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
		   "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
		   "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
		   "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
		   "OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
		   "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n", now->tm_year+1900);
	
	printf("The following parts of the Software are separately licensed:\n"
		   "- The Liberation font family is licensed under the SIL Open Font License" "(version 2 onwards)\n\n");
}

static void print_help_message()
{
#define explain_required_argument(c,e) printf("   %-18s%-18s%s\n", c, "REQUIRED", e);
#define explain_other_argument(c,e) printf("   %-18s%-18s%s\n", c, " ", e);
#define explain_optional_argument(c,d,e) printf("   %-18s%-18s%s\n", c, d, e);
#define DEFAULT(d) "default=\""d"\""
	
	printf("Usage: text-search [OPTION] ... [OPTION] ...\n");
	printf("Example: text-search "
		   "--directory \"/home/john/Documents\" --text \"homework\" --recursive 0\n");
	printf("Text-search, search for files and text within files.\n");
	printf("Matches will be printed to console in format: [path]:[line]:[filter]\n\n");
	
	printf("Available arguments:\n");
	explain_required_argument("--directory", "The directory to search");
	explain_optional_argument("--text", DEFAULT("*"), "The text to search for within files, supports wildcards '*' and '?'");
	explain_optional_argument("--filter", DEFAULT("*"), "Used to filter on specific files,  supports wildcards '*' and '?'");
	explain_optional_argument("--recursive", DEFAULT("1"), "Recursively search through directories");
	explain_optional_argument("--max-file-size", DEFAULT("0"), "The maximum size, in kb, a file will be searched through for matching text. 0 for no limit");
	explain_optional_argument("--threads", DEFAULT("10"), "The number of threads used for searching, minimum of 1 thread.");
	explain_optional_argument("--locale", DEFAULT("en"), "The language errors will be reported in. Available locales are: 'en', 'nl'");
	explain_optional_argument("--export", DEFAULT(""), "Export the results to a file in json format");
	
	printf("\nOther arguments:\n");
	explain_other_argument("--help", "Display this help message");
	explain_other_argument("--license", "Display the license");
}

static bool is_valid_argument(char *arg)
{
	if (string_equals(arg, "--directory")) return true;
	if (string_equals(arg, "--text")) return true;
	if (string_equals(arg, "--filter")) return true;
	if (string_equals(arg, "--recursive")) return true;
	if (string_equals(arg, "--max-file-size")) return true;
	if (string_equals(arg, "--threads")) return true;
	if (string_equals(arg, "--locale")) return true;
	if (string_equals(arg, "--export")) return true;
	
	return false;
}

void handle_command_line_arguments(int argc, char **argv)
{
	load_available_localizations();
	set_locale("en");
	
	s32 current_arg_index = 1;
	bool is_help_request = string_equals(argv[current_arg_index], "--help");
	bool is_license_request = string_equals(argv[current_arg_index], "--license");
	
	if (is_help_request)
	{
		print_help_message();
		return;
	}
	else if (is_license_request)
	{
		print_license_message();
		return;
	}
	
	char directory[MAX_INPUT_LENGTH];
	string_copyn(directory, "", MAX_INPUT_LENGTH);
	
	char text[MAX_INPUT_LENGTH];
	string_copyn(text, "*", MAX_INPUT_LENGTH);
	
	char filter[MAX_INPUT_LENGTH];
	string_copyn(filter, "*", MAX_INPUT_LENGTH);
	
	bool recursive = true;
	s32 max_file_size = 0;
	s32 threads = 10;
	
	char locale[MAX_INPUT_LENGTH];
	string_copyn(locale, "en", MAX_INPUT_LENGTH);
	
	char export_path[MAX_INPUT_LENGTH];
	string_copyn(export_path, "", MAX_INPUT_LENGTH);
	
	bool expect_argument_name = true;
	for (s32 i = current_arg_index; i < argc; i++)
	{
		if (expect_argument_name && !is_valid_argument(argv[i]))
		{
			printf("%s: %s\n", localize("invalid_argument"), argv[i]);
		}
		
		if (!expect_argument_name)
		{
			if (string_equals(argv[i-1], "--directory"))
			{
				string_copyn(directory, argv[i], MAX_INPUT_LENGTH);
			}
			if (string_equals(argv[i-1], "--text"))
			{
				string_copyn(text, argv[i], MAX_INPUT_LENGTH);
			}
			if (string_equals(argv[i-1], "--filter"))
			{
				string_copyn(filter, argv[i], MAX_INPUT_LENGTH);
			}
			if (string_equals(argv[i-1], "--recursive"))
			{
				recursive = string_to_u32(argv[i]);
			}
			if (string_equals(argv[i-1], "--max-file-size"))
			{
				max_file_size = string_to_u32(argv[i]);
			}
			if (string_equals(argv[i-1], "--threads"))
			{
				threads = string_to_u32(argv[i]);
			}
			if (string_equals(argv[i-1], "--locale"))
			{
				string_copyn(locale, argv[i], MAX_INPUT_LENGTH);
			}
			if (string_equals(argv[i-1], "--export"))
			{
				string_copyn(export_path, argv[i], MAX_INPUT_LENGTH);
			}
		}
		
		expect_argument_name = !expect_argument_name;
	}
	
	// input validation
	if (!set_locale(locale))
	{
		printf(localize("warning_locale_not_available"), locale);
		printf("\n");
	}
	
	if (string_equals(directory, ""))
	{
		printf("%s", localize("error_directory_not_specified"));
		printf("\n");
		return;
	}
	
	if (!platform_directory_exists(directory))
	{
		printf(localize("error_directory_not_found"), directory);
		printf("\n");
		return;
	}
	
	if (string_equals(text, ""))
	{
		printf("%s", localize("error_text_argument_empty"));
		printf("\n");
		return;
	}
	
	if (string_equals(filter, ""))
	{
		printf("%s", localize("error_filter_argument_empty"));
		printf("\n");
		return;
	}
	
	if (threads < 1)
	{
		printf("%s", localize("error_threads_too_low"));
		printf("\n");
		return;
	}
	
	if (!string_equals(export_path, ""))
	{
		char dir_buffer[MAX_INPUT_LENGTH];
		get_directory_from_path(dir_buffer, export_path);
		
		if (!platform_directory_exists(dir_buffer))
		{
			printf(localize("error_invalid_export_path"), dir_buffer);
			printf("\n");
			return;
		}
	}
	
	search_result *result = create_empty_search_result();
	string_copyn(result->directory_to_search, directory, MAX_INPUT_LENGTH);
	string_copyn(result->file_filter, filter, MAX_INPUT_LENGTH);
	string_copyn(result->text_to_find, text, MAX_INPUT_LENGTH);
	string_copyn(result->export_path, export_path, MAX_INPUT_LENGTH);
	result->max_thread_count = threads;
	result->max_file_size = max_file_size;
	result->is_recursive = recursive;
	result->is_command_line_search = true;
	
	
	// begin search (code below is equal to code in text_search.c)
	result->walking_file_system = true;
	result->done_finding_matches = false;
	
	result->search_result_source_dir_len = strlen(result->directory_to_search);
	result->search_result_source_dir_len = prepare_search_directory_path(result->directory_to_search, 
																		 result->search_result_source_dir_len);
	result->start_time = platform_get_time(TIME_FULL, TIME_US);
	
	platform_list_files(&result->files, result->directory_to_search, result->file_filter, result->is_recursive, &result->mem_bucket,
						&result->cancel_search,
						&result->done_finding_files);
	find_text_in_files(result);
	
	while(!result->threads_closed) { thread_sleep(1000); }
	
	if (!string_equals(export_path, ""))
	{
		export_results(result);
	}
}