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

static void print_license_message()
{
	printf("License: XD!\n");
}

static void print_help_message()
{
#define explain_required_command(c,e) printf("   %-18s%-18s%s\n", c, "REQUIRED", e);
#define explain_other_command(c,e) printf("   %-18s%-18s%s\n", c, " ", e);
#define explain_optional_command(c,d,e) printf("   %-18s%-18s%s\n", c, d, e);
#define DEFAULT(d) "default=\""d"\""
	
	printf("Usage: text-search [OPTION] ... [OPTION] ...\n");
	printf("Example: text-search "
		   "--directory \"/home/john/Documents\" --text \"homework\"\n");
	printf("Text-search, search for files and text within files.\n");
	printf("Matches will be printed to console in format: [path]:[line]:[filter]\n\n");
	
	printf("Available options:\n");
	explain_required_command("--directory", "The directory to search");
	explain_optional_command("--text", DEFAULT("*"), "The text to search for within files, supports wildcards '*' and '?'");
	explain_optional_command("--filter", DEFAULT("*"), "Used to filter on specific files,  supports wildcards '*' and '?'");
	explain_optional_command("--recursive", DEFAULT("1"), "Recursively search through directories");
	explain_optional_command("--max-file-size", DEFAULT("0"), "The maximum size, in kb, a file will be searched through for matching text. 0 for no limit");
	explain_optional_command("--threads", DEFAULT("10"), "The number of threads used for searching");
	explain_optional_command("--locale", DEFAULT("en"), "The language errors will be reported in. Available locales are: 'en', 'nl'");
	explain_optional_command("--export", DEFAULT(""), "Export the results to a file in json format");
	explain_optional_command("--export", DEFAULT(""), "Export the results to a file in json format");
	
	printf("\nOther commands:\n");
	explain_other_command("--help", "Display this help message");
	explain_other_command("--license", "Display the license");
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
}