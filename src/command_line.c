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

void find_text_in_files(search_result *search_result);
s32 prepare_search_directory_path(char *path, s32 len);
search_result *create_empty_search_result();
void do_search();

static void print_license_message()
{
	// TODO(Aldrik): license
	printf("License: XD!\n");
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
			// TODO(Aldrik): localize
			printf("Invalid argument: %s\n", argv[i]);
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
	load_available_localizations();
	if (!set_locale(locale))
	{
		// TODO(Aldrik): localize
		printf("WARNING: '--locale' argument invalid: locale '%s' not available, "
			   "defaulting to english\n", locale);
	}
	
	if (string_equals(directory, ""))
	{
		// TODO(Aldrik): localize
		printf("ERROR: '--directory' option is a required argument\n");
		return;
	}
	
	if (!platform_directory_exists(directory))
	{
		// TODO(Aldrik): localize
		printf("ERROR: Directory provided in option '--directory' "
			   "does not exist: '%s'\n", directory);
		return;
	}
	
	if (string_equals(text, ""))
	{
		// TODO(Aldrik): localize
		printf("ERROR: '--text' argument cannot be empty\n");
		return;
	}
	
	if (string_equals(filter, ""))
	{
		// TODO(Aldrik): localize
		printf("ERROR: '--filter' argument cannot be empty\n");
		return;
	}
	
	if (threads < 1)
	{
		// TODO(Aldrik): localize
		printf("ERROR: '--threads' needs to be greater than 0\n");
		return;
	}
	
	if (!string_equals(export_path, ""))
	{
		char dir_buffer[MAX_INPUT_LENGTH];
		get_directory_from_path(dir_buffer, export_path);
		
		if (!platform_directory_exists(dir_buffer))
		{
			// TODO(Aldrik): localize
			printf("ERROR: '--export' invalid path. Directory to save "
				   "file to does not exist: '%s'\n", dir_buffer);
		}
		return;
	}
	
	search_result *result = create_empty_search_result();
	string_copyn(result->directory_to_search, directory, MAX_INPUT_LENGTH);
	string_copyn(result->file_filter, filter, MAX_INPUT_LENGTH);
	string_copyn(result->text_to_find, text, MAX_INPUT_LENGTH);
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
	
	while(!result->done_finding_matches) { thread_sleep(1000); }
}