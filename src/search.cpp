#include "search.h"
#include "platform.h"

#include <stdio.h>

ts_search_result *current_search_result = nullptr;

ts_array ts_get_filters(char *pattern)
{
	ts_array result = ts_array_create(MAX_INPUT_LENGTH);

	char current_filter[MAX_INPUT_LENGTH];
	int filter_len = 0;
	while (*pattern)
	{
		char ch = *pattern;

		if (ch == ',')
		{
			current_filter[filter_len] = 0;
			ts_array_push(&result, current_filter);
			filter_len = 0;
		}
		else
		{
			if (filter_len < MAX_INPUT_LENGTH - 1)
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
	ts_array_push(&result, current_filter);

	return result;
}

int ts_string_match(char *first, char *second)
{
	// If we reach at the end of both strings, we are done
	if (*first == '\0' && *second == '\0')
		return 1;

	// Make sure that the characters after '*' are present
	// in second string. This function assumes that the first
	// string will not contain two consecutive '*'
	if (*first == '*' && *(first + 1) != '\0' && *second == '\0')
		return 0;

	// If the first string contains '?', or current characters
	// of both strings string_match
	if (*first == '?' || *first == *second)
		return ts_string_match(first + 1, second + 1);

	// If there is *, then there are two possibilities
	// a) We consider current character of second string
	// b) We ignore current character of second string.
	if (*first == '*')
		return ts_string_match(first + 1, second) || ts_string_match(first, second + 1);
	return 0;
}

int ts_filter_matches(ts_array *filters, char *string, char **matched_filter)
{
	for (int i = 0; i < filters->length; i++)
	{
		char *filter = (char *)ts_array_at(filters, i);

		char wildcard_filter[MAX_INPUT_LENGTH];
		snprintf(wildcard_filter, MAX_INPUT_LENGTH, "*%s", filter);
		if (ts_string_match(wildcard_filter, string))
		{
			*matched_filter = filter;
			return strlen(filter);
		}
	}
	return -1;
}

ts_search_result *ts_create_empty_search_result()
{
	ts_search_result *new_result_buffer = (ts_search_result *)malloc(sizeof(ts_search_result));
	new_result_buffer->completed_match_threads = 0;
	new_result_buffer->mutex = ts_mutex_create();
	new_result_buffer->done_finding_files = false;
	new_result_buffer->file_list_read_cursor = 0;
	new_result_buffer->max_ts_thread_count = 1;
	new_result_buffer->match_count = 0;
	new_result_buffer->search_completed = false;
	new_result_buffer->file_count = 0;
	new_result_buffer->cancel_search = false;
	new_result_buffer->max_file_size = megabytes(1000);
	new_result_buffer->memory = ts_memory_bucket_init(megabytes(1));
	new_result_buffer->prev_result = current_search_result;

	new_result_buffer->files = ts_array_create(sizeof(ts_found_file));
	new_result_buffer->files.reserve_jump = FILE_RESERVE_COUNT;
	ts_array_reserve(&new_result_buffer->files, FILE_RESERVE_COUNT);

	new_result_buffer->matches = ts_array_create(sizeof(ts_file_match));
	new_result_buffer->matches.reserve_jump = FILE_RESERVE_COUNT;
	ts_array_reserve(&new_result_buffer->matches, FILE_RESERVE_COUNT);

	// filter buffers
	new_result_buffer->directory_to_search = (char *)ts_memory_bucket_reserve(&new_result_buffer->memory, MAX_INPUT_LENGTH);
	new_result_buffer->search_text = (char *)ts_memory_bucket_reserve(&new_result_buffer->memory, MAX_INPUT_LENGTH);

	return new_result_buffer;
}

bool string_is_asteriks(char *text)
{
	utf8_int32_t ch;
	while ((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch != '*')
			return false;
	}
	return true;
}

bool ts_string_contains(char *text_to_search, utf8_int8_t *text_to_find, ts_array *text_matches)
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
	while (utf8codepoint(text_to_find, &br) && br == '*')
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
	char *line_start_ptr = text_to_search;

	int index = 0;
	while ((text_to_search = utf8codepoint(text_to_search, &text_to_search_ch)) && text_to_search_ch)
	{
		word_offset_val += utf8codepointsize(text_to_search_ch);
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
		// text_to_search_current_attempt = utf8codepoint(text_to_search_current_attempt,
		//&text_to_search_current_attempt_ch);

		word_match_len_val = 0;
		while (text_to_search_current_attempt_ch)
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

			// character does not match, continue search
			if (text_to_find_ch != text_to_search_current_attempt_ch && !in_wildcard)
				break;

		continue_search:
			if (!in_wildcard)
				text_to_find = utf8codepoint(text_to_find, &text_to_find_ch);

			word_match_len_val += utf8codepointsize(text_to_search_current_attempt_ch);
			text_to_search_current_attempt = utf8codepoint(
				text_to_search_current_attempt,
				&text_to_search_current_attempt_ch);

			if (!text_to_search_current_attempt_ch && !text_to_find_ch)
				goto done;


			// text to find has reached 0byte, word has been found
			if (text_to_find_ch == 0)
			{
			done:
				if (save_info)
				{
					ts_text_match new_match;
					new_match.line_nr = line_nr_val;
					new_match.word_offset = word_offset_val - utf8codepointsize(text_to_search_ch); // first codepoint was also added..
					new_match.word_match_len = word_match_len_val;
					new_match.line_start = line_start_ptr;
					new_match.line_info = 0;
					ts_array_push(text_matches, &new_match);
				}

				final_result = true;

				if (is_asteriks_only)
				{
					return final_result;
				}

				break;
			}
		}

		text_to_find = text_to_find_original;
		index++;
	}

	return final_result;
}

static void _ts_search_file(ts_found_file *ref, ts_file_content content, ts_search_result *result)
{
	if (content.content && !content.file_error)
	{
		ts_array text_matches = ts_array_create(sizeof(ts_text_match));
		int search_len = strlen(result->search_text);
		if (ts_string_contains((char *)content.content, result->search_text, &text_matches))
		{
			ts_mutex_lock(&result->matches.mutex);
			for (int i = 0; i < text_matches.length; i++)
			{
				ts_text_match *m = (ts_text_match *)ts_array_at(&text_matches, i);

				ts_file_match file_match;
				file_match.file = ref;
				file_match.line_nr = m->line_nr;
				file_match.word_match_offset = m->word_offset;
				file_match.word_match_length = m->word_match_len;
				file_match.line_info = (char *)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);

				int text_pad_lr = 25;
				if (file_match.word_match_offset > text_pad_lr)
				{
					m->line_start += file_match.word_match_offset - text_pad_lr;
					file_match.word_match_offset = text_pad_lr;
				}
				int total_len = text_pad_lr + search_len + text_pad_lr;
				if (total_len > MAX_INPUT_LENGTH) total_len = MAX_INPUT_LENGTH;
				utf8ncpy(file_match.line_info, m->line_start, total_len);

				utf8_int32_t ch;
				utf8_int8_t* iter = file_match.line_info;
				while ((iter = utf8codepoint(iter, &ch)) && ch)
				{
					if (ch == '\n') iter[-1] = ' ';
					if (ch == '\t') iter[-1] = ' ';
					if (ch == '\r') iter[-1] = ' ';
					if (ch == '\x0B') iter[-1] = ' ';
				}

				ts_array_push_size(&result->matches, &file_match, sizeof(file_match));
				ref->match_count++;
				result->match_count = result->matches.length;
			}
			ts_mutex_unlock(&result->matches.mutex);
		}

		ts_array_destroy(&text_matches);
	}
}

static void *_ts_search_thread(void *args)
{
	ts_search_result *new_result = (ts_search_result *)args;
	if (new_result->search_text == nullptr) goto finish_early;

keep_going:;
	while (new_result->file_list_read_cursor < new_result->files.length)
	{
		if (new_result->cancel_search)
			goto finish_early;

		ts_mutex_lock(&new_result->files.mutex);
		int read_cursor = new_result->file_list_read_cursor++;
		new_result->file_count++;
		ts_mutex_unlock(&new_result->files.mutex);

		if (read_cursor >= new_result->files.length)
			continue;

		ts_found_file *f = *(ts_found_file **)ts_array_at(&new_result->files, read_cursor);
		ts_file_content content = ts_platform_read_file(f->path, "rb, ccs=UTF-8");

		_ts_search_file(f, content, new_result);

		free(content.content);
	}

	if (!new_result->done_finding_files)
		goto keep_going;

finish_early:;
	ts_mutex_lock(&new_result->files.mutex);
	new_result->completed_match_threads++;
	ts_mutex_unlock(&new_result->files.mutex);

	return 0;
}

static void *_ts_list_files_thread(void *args)
{
	ts_search_result *info = (ts_search_result *)args;
	ts_platform_list_files_block(info, nullptr);
	info->done_finding_files = true;

	// Use this thread to cleanup previous result.
	if (info->prev_result) {
		while (!info->prev_result->search_completed) {
			ts_thread_sleep(10);
		}
		ts_memory_bucket_destroy(&info->prev_result->memory);
		free(info->prev_result);
		info->prev_result = nullptr;
	}

	// Use this thread to sync.
	while (!info->search_completed) {
		if (info->completed_match_threads == info->max_ts_thread_count) {
			info->search_completed = true; // No memory is written after this point.
		}
		ts_thread_sleep(10);
	}

	return 0;
}

static void _ts_list_files(ts_search_result* result)
{
	ts_thread thr = ts_thread_start(_ts_list_files_thread, (void*)result);
	ts_thread_detach(&thr);
}

void ts_start_search(utf8_int8_t *path, utf8_int8_t *filter, utf8_int8_t *query)
{
	if (utf8len(query) > 0 && utf8len(query) <= 2) { // need a string of atleast 3 characters
		return;
	}

	if (current_search_result)
	{
		current_search_result->cancel_search = true;
	}

	ts_search_result *new_result = ts_create_empty_search_result();
	snprintf(new_result->directory_to_search, MAX_INPUT_LENGTH, "%s", path);
	snprintf(new_result->search_text, MAX_INPUT_LENGTH, "%s", query);
	new_result->filters = ts_get_filters(filter);

	if (utf8len(query) == 0) {
		new_result->search_text = nullptr;
	}

	_ts_list_files(new_result);
	for (int i = 0; i < new_result->max_ts_thread_count; i++)
	{
		ts_thread thr = ts_thread_start(_ts_search_thread, new_result);
		ts_thread_detach(&thr);
	}

	current_search_result = new_result;
}