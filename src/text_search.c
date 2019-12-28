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

#include "config.h"
#include "project_base.h"

typedef struct t_file_match
{
	found_file file;
	s16 file_error;
	s32 file_size;
	
	u32 line_nr;
	s32 word_match_offset_x;
	s32 word_match_width;
	char *line_info; // will be null when no match is found
} file_match;

typedef struct t_status_bar
{
	char *result_status_text;
	char *error_status_text;
} status_bar;

typedef struct t_search_result
{
	array work_queue;
	array files;
	array matches;
	u64 find_duration_us;
	array errors;
	bool show_error_message; // error occured
	bool found_file_matches; // found/finding file matches
	s32 files_searched;
	s32 files_matched;
	s32 search_result_source_dir_len;
	bool match_found; // found text match
	mutex mutex;
	bool walking_file_system;
	bool cancel_search;
	bool done_finding_matches;
	char *filter_buffer;
	char *text_to_find_buffer;
	char *search_directory_buffer;
	bool *recursive_buffer;
	s32 search_id;
	u64 start_time;
	bool done_finding_files;
	char *text_to_find;
	memory_bucket mem_bucket;
	s32 max_thread_count;
	s32 max_file_size;
	bool is_recursive;
} search_result;

typedef struct t_find_text_args
{
	file_match file;
	search_result *search_result_buffer;
} find_text_args;

status_bar global_status_bar;
search_result *current_search_result;

image *search_img;
image *error_img;
image *directory_img;
image *logo_small_img;

font *font_big;
font *font_medium;
font *font_small;
font *font_mini;
s32 scroll_y = 0;

platform_window *main_window;

#include "save.h"
#include "settings.h"

#include "save.c"
#include "settings.c"

// TODO(Aldrik): highlight matched text in line_info
// TODO(Aldrik): textbox sometimes doesnt take input?
// TODO(Aldrik): double click to select path/path:line/path:line:match_text/path:line:matched_filter:matched_text config option
// TODO(Aldrik): redo (ctrl+y)
// TODO(Aldrik): capture mouse position outside of window on windows so that we can drag scrollbar outside of window
// TODO(Aldrik): colors of icon are wrong
// TODO(Aldrik): name of application in taskbar on linux
// TODO(Aldrik): multiple results in file?
// TODO(Aldrik): thoroughly test it, make a fuzzer or something
// TODO(Aldrik): save as dialog on windows not showing available file types
// TODO(Aldrik): localize hardcoded strings ("style","no search completed","Cancelling search","Copy config path to clipboard")
// TODO(Aldrik): command line usage

checkbox_state checkbox_recursive;
textbox_state textbox_search_text;
textbox_state textbox_path;
textbox_state textbox_file_filter;
button_state button_select_directory;
button_state button_find_text;
button_state button_cancel;

void* destroy_search_result_thread(void *arg)
{
	search_result *buffer = arg;
	
	// wait 3 sec for all threads writing to memory bucket to finish
	thread_sleep(1000*1000*3);
	
	array_destroy(&buffer->matches);
	array_destroy(&buffer->errors);
	
	memory_bucket_destroy(&buffer->mem_bucket);
	
	mutex_destroy(&buffer->mutex);
	mem_free(buffer);
	
	return 0;
}

static void* find_text_in_file_worker(void *arg)
{
	search_result *result_buffer = arg;
	
	file_content content;
	content.content = 0;
	
	while(!result_buffer->done_finding_matches)
	{
		if (result_buffer->cancel_search) { goto finish_early; }
		
		mutex_lock(&result_buffer->work_queue.mutex);
		if (result_buffer->work_queue.length)
		{
			find_text_args args = *(find_text_args*)array_at(&result_buffer->work_queue, 0);
			array_remove_at(&result_buffer->work_queue, 0);
			mutex_unlock(&result_buffer->work_queue.mutex);
			
			// read file
			content = platform_read_file_content(args.file.file.path, "r");
			args.file.file_size = content.content_length;
			
			// check if file is not too big for set filter
			s32 kb_to_b = result_buffer->max_file_size * 1000;
			if (result_buffer->max_file_size && content.content_length > kb_to_b)
			{
				mutex_lock(&result_buffer->mutex);
				result_buffer->files_searched++;
				mutex_unlock(&result_buffer->mutex);
				
				platform_destroy_file_content(&content);
				continue;
			}
			
			// check if file has opened correctly
			if (content.content && !content.file_error)
			{
				array text_matches = array_create(sizeof(text_match));
				
				if (string_contains_ex(content.content, result_buffer->text_to_find, &text_matches, &result_buffer->cancel_search))
				{
					args.search_result_buffer->match_found = true;
					
					for (s32 i = 0; i < text_matches.length; i++)
					{
						text_match *m = array_at(&text_matches, i);
						file_match file_match = args.file;
						
						// match info
						file_match.line_nr = m->line_nr;
						file_match.line_info = memory_bucket_reserve(&result_buffer->mem_bucket, 120); // show 20 chars behind text match. + 10 extra space
						
						s32 offset_to_render = m->word_offset;// < 20 ? m->word_offset : 
						//m->word_offset < 20 - 20;
						
						char *str_to_copy = m->line_start;//offset_to_render < 20 ?
						//m->line_start : m->line_start+offset_to_render-20;
						
						file_match.word_match_offset_x = 
							calculate_text_width_upto(font_small, str_to_copy, offset_to_render-1);
						
						file_match.word_match_width = 50;
						
						sprintf(file_match.line_info, "%.40s", str_to_copy);
						char *tmp = file_match.line_info;
						while(*tmp)
						{
							if (*tmp == '\n')
							{
								*tmp = 0;
								break;
							}
							++tmp;
						}
						
						array_push(&result_buffer->matches, &file_match);
					}
					
					
					mutex_lock(&result_buffer->mutex);
					result_buffer->files_matched++;
					mutex_unlock(&result_buffer->mutex);
				}
				
				array_destroy(&text_matches);
			}
			else
			{
				if (content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS ||
					content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM)
				{
					// TODO(Aldrik): maybe retry here
				}
				
				if (content.file_error)
					args.file.file_error = content.file_error;
				else
					args.file.file_error = FILE_ERROR_GENERIC;
				
				mutex_lock(&args.search_result_buffer->mutex);
				strncpy(global_status_bar.error_status_text, localize("generic_file_open_error"), MAX_ERROR_MESSAGE_LENGTH);
				mutex_unlock(&args.search_result_buffer->mutex);
			}
			
			mutex_lock(&result_buffer->mutex);
			result_buffer->files_searched++;
			mutex_unlock(&result_buffer->mutex);
			
			platform_destroy_file_content(&content);
		}
		else
		{
			mutex_unlock(&result_buffer->work_queue.mutex);
		}
	}
	
	if (!result_buffer->cancel_search)
	{
		mutex_lock(&result_buffer->mutex);
		sprintf(global_status_bar.result_status_text, localize("percentage_files_processed"),  (result_buffer->files_searched/(float)result_buffer->files.length)*100);
		mutex_unlock(&result_buffer->mutex);
	}
	
	finish_early:;
	return 0;
}

static void* find_text_in_files_t(void *arg)
{
	search_result *result_buffer = arg;
	s32 current_search_id = result_buffer->search_id;
	
	array threads = array_create(sizeof(thread));
	array_reserve(&threads, result_buffer->max_thread_count);
	
	strncpy(global_status_bar.error_status_text, "", MAX_ERROR_MESSAGE_LENGTH);
	char *text_to_find = result_buffer->text_to_find;
	
	// create worker threads
	for(s32 i = 0; i < result_buffer->max_thread_count; i++)
	{
		thread new_thr = thread_start(find_text_in_file_worker, result_buffer);
		array_push(&threads, &new_thr);
	}
	
	s32 start = 0;
	s32 len = 0;
	do_work:
	{
		if (result_buffer->cancel_search) 
		{
			goto finish_early;
		}
		
		mutex_lock(&result_buffer->files.mutex);
		len = result_buffer->files.length;
		for (s32 i = start; i < len; i++)
		{
			find_text_args args;
			args.file = *(file_match*)array_at(&result_buffer->files, i);
			args.file.file_error = 0;
			args.file.file_size = 0;
			args.file.line_info = 0;
			args.search_result_buffer = result_buffer;
			
			mutex_lock(&result_buffer->work_queue.mutex);
			array_push(&result_buffer->work_queue, &args);
			mutex_unlock(&result_buffer->work_queue.mutex);
		}
		mutex_unlock(&result_buffer->files.mutex);
	}
	
	// check if there are files not in queue yet
	{
		mutex_lock(&result_buffer->files.mutex);
		if (result_buffer->done_finding_files)
		{
			if (len != result_buffer->files.length)
			{
				start = len;
				len = result_buffer->files.length;
				mutex_unlock(&result_buffer->files.mutex);
				goto do_work;
			}
			mutex_unlock(&result_buffer->files.mutex);
		}
		else
		{
			start = len;
			len = result_buffer->files.length;
			mutex_unlock(&result_buffer->files.mutex);
			thread_sleep(100);
			goto do_work;
		}
	}
	
	//thread_sleep(15000);
	
	// wait untill queue is cleared
	while(result_buffer->work_queue.length) 
	{
		//printf("in queue: %d, cancelled: %d\n", result_buffer->work_queue.length, result_buffer->cancel_search);
		if (result_buffer->cancel_search) 
		{
			goto finish_early;
		}
		thread_sleep(100);
	}
	
	//thread_sleep(15000);
	
	finish_early:
	{
		u64 end_f = platform_get_time(TIME_FULL, TIME_US);
		
		result_buffer->find_duration_us = end_f - result_buffer->start_time;
		result_buffer->done_finding_matches = true;
		result_buffer->walking_file_system = false;
		result_buffer->files_searched = result_buffer->files.length;
		
		if (!main_window->has_focus)
			platform_show_alert("Text-search", localize("search_result_completed"));
		
	}
	
	for (s32 i = 0; i < threads.length; i++)
	{
		thread *thr = array_at(&threads, i);
		thread_join(thr);
	}
	
	// TODO(Aldrik): LOCALIZE!
	sprintf(global_status_bar.result_status_text, "%d matches found in %d files, %.2fms", result_buffer->matches.length, result_buffer->files.length, result_buffer->find_duration_us/1000.0);
	
	array_destroy(&threads);
	array_destroy(&result_buffer->work_queue);
	array_destroy(&result_buffer->files);
	
	return 0;
}

static void find_text_in_files(search_result *search_result)
{
	search_result->files_matched = 0;
	search_result->files_searched = 0;
	search_result->found_file_matches = true;
	search_result->match_found = false;
	
	thread thr = thread_start(find_text_in_files_t, search_result);
	thread_detach(&thr);
}

static void render_status_bar(platform_window *window, font *font_small)
{
	s32 h = 30;
	s32 y = window->height - h;
	s32 img_size = 16;
	
	// result status
	s32 text_size = calculate_text_width(font_small, global_status_bar.result_status_text);
	render_rectangle(-1, y, window->width+2, h, global_ui_context.style.info_bar_background);
	render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
	render_set_scissor(main_window, main_window->width/2, y, main_window->width/2, h);
	render_text(font_small, window->width - text_size - 8, y + (h/2)-(font_small->size/2) + 1, global_status_bar.result_status_text, global_ui_context.style.foreground);
	render_reset_scissor(main_window);
	
	// error status
	if (global_status_bar.error_status_text[0] != 0)
	{
		render_set_scissor(main_window, 0, y, main_window->width/2, h);
		render_image(error_img, 6, y + (h/2) - (img_size/2), img_size, img_size);
		render_text(font_small, 12 + img_size, y + (h/2)-(font_small->size/2) + 1, global_status_bar.error_status_text, global_ui_context.style.error_foreground);
		render_reset_scissor(main_window);
	}
}

static void set_status_text_to_active()
{
	char text[40];
	
	u64 dot_count_t = platform_get_time(TIME_FULL, TIME_MILI_S);
	s32 dot_count = (dot_count_t % 1000) / 250;
	
	sprintf(text, "%.*s%s", dot_count, "...", localize("finding_files"));
	
	strncpy(global_status_bar.result_status_text, text, MAX_STATUS_TEXT_LENGTH);
}

static void set_status_text_to_cancelled()
{
	strncpy(global_status_bar.result_status_text, "Cancelling search", MAX_STATUS_TEXT_LENGTH);
}

static void reset_status_text()
{
	strncpy(global_status_bar.result_status_text, localize("no_search_completed"), MAX_STATUS_TEXT_LENGTH);
}

static void render_update_result(platform_window *window, font *font_small, mouse_input *mouse, keyboard_input *keyboard)
{
	if (!current_search_result->done_finding_matches)
	{
		if (!current_search_result->cancel_search)
			set_status_text_to_active();
		else
			set_status_text_to_cancelled();
	}
	
	s32 y = global_ui_context.layout.offset_y;
	s32 h = 24;
	
	s32 render_y = y - WIDGET_PADDING;
	s32 render_h;
	render_h = window->height - render_y - 10;
	
	render_set_scissor(window, 0, render_y, window->width, render_h);
	
	if (current_search_result->match_found)
	{
		y -= 9;
		
		s32 path_width = window->width / 2.0;
		s32 pattern_width = window->width / 8.0;
		
		/// header /////////////
		render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
		
		render_rectangle(-1, y+1, window->width+2, h-2, global_ui_context.style.info_bar_background);
		
		render_text(font_small, 10, y + (h/2)-(font_small->size/2) + 1, localize("file_path"), global_ui_context.style.foreground);
		
		render_text(font_small, 10 + path_width, y + (h/2)-(font_small->size/2) + 1, localize("file_pattern"), global_ui_context.style.foreground);
		
		render_text(font_small, 10 + path_width + pattern_width, y + (h/2)-(font_small->size/2) + 1, localize("information"), global_ui_context.style.foreground);
		/////////////////////////
		
		y += h-1;
		
		s32 scroll_w = 14;
		s32 total_h = 0;
		s32 start_y = y;
		s32 total_space = window->height - start_y - 30 + 1;
		render_set_scissor(window, 0, y, window->width, render_h - 43);
		
		/// draw entries ////////
		s32 drawn_entity_count = 0;
		for (s32 i = 0; i < current_search_result->matches.length; i++)
		{
			file_match *match = array_at(&current_search_result->matches, i);
			
			if (match->line_info || match->file_error)
			{
				drawn_entity_count++;
				s32 rec_y = y+scroll_y;
				
				if (rec_y > start_y - h && rec_y < start_y + total_space)
				{
#if 1
					// hover item and click item
					if (mouse->y > rec_y && mouse->y < rec_y + h && mouse->y < window->height - 30 &&
						mouse->x >= 0 && mouse->x < window->width - scroll_w)
					{
						if (is_left_double_clicked(mouse))
						{
							platform_set_clipboard(main_window, match->file.path);
							//show_notification("Path copied to clipboard");
						}
						
						render_rectangle(-1, rec_y, window->width+2, h, global_ui_context.style.item_hover_background);
						platform_set_cursor(window, CURSOR_POINTER);
					}
#endif
					
					// outline
					render_rectangle_outline(-1, rec_y, window->width+2, h, 1, global_ui_context.style.border);
					
					// path
					render_set_scissor(window, 0, start_y, path_width-10, render_h - 43);
					render_text(font_small, 10, rec_y + (h/2)-(font_small->size/2) + 1, match->file.path + current_search_result->search_result_source_dir_len, global_ui_context.style.foreground);
					
					// pattern
					render_set_scissor(window, 0, start_y, 
									   path_width+pattern_width-10, render_h - 43);
					render_text(font_small, 10 + path_width, rec_y + (h/2)-(font_small->size/2) + 1, match->file.matched_filter, global_ui_context.style.foreground);
					
					// state
					render_set_scissor(window, 0, start_y, window->width, render_h - 43);
					if (!match->file_error && match->line_info)
					{
						s32 text_sx = 10 + path_width + pattern_width;
						s32 text_sy = rec_y + (h/2)-(font_small->size/2) + 1;
						
						char tmp[80];
						sprintf(tmp, "line %d: ", match->line_nr);
						
						text_sx += render_text(font_small, text_sx, text_sy, 
											   tmp, global_ui_context.style.foreground);
						
						// highlight matched text
						render_rectangle(text_sx + match->word_match_offset_x, text_sy, match->word_match_width, 10, rgb(255,0,0));
						
						render_text(font_small, text_sx, text_sy, 
									match->line_info, global_ui_context.style.foreground);
					}
					else
					{
						s32 img_size = 14;
						render_image(error_img, 6 + path_width + pattern_width, rec_y + (h/2) - (img_size/2), img_size, img_size);
						
						char *open_file_error = 0;
						switch(match->file_error)
						{
							case FILE_ERROR_NO_ACCESS: open_file_error = localize("no_permission"); break;
							case FILE_ERROR_NOT_FOUND: open_file_error = localize("not_found"); break;
							case FILE_ERROR_CONNECTION_ABORTED: open_file_error = localize("connection_aborted"); break;
							case FILE_ERROR_CONNECTION_REFUSED: open_file_error = localize("connection_refused"); break;
							case FILE_ERROR_NETWORK_DOWN: open_file_error = localize("network_down"); break;
							case FILE_ERROR_REMOTE_IO_ERROR: open_file_error = localize("remote_error"); break;
							case FILE_ERROR_STALE: open_file_error = localize("remotely_removed"); break;
							default:
							case FILE_ERROR_GENERIC: open_file_error = localize("failed_to_open_file"); break;
						}
						
						render_text(font_small, 10 + path_width + pattern_width + img_size + 6, rec_y + (h/2)-(font_small->size/2) + 1, open_file_error, ERROR_TEXT_COLOR);
					}
				}
				y += h-1;
				total_h += h-1;
			}
		}
		////////////////////////
		
		s32 overflow = total_h - total_space;
		
		/// scrollbar //////////
		if (overflow > 0)
		{
			if (global_ui_context.mouse->y >= start_y && global_ui_context.mouse->y <= start_y + total_space)
			{
				// scroll with mouse
				{
					if (global_ui_context.mouse->scroll_state == SCROLL_UP)
						scroll_y+=(h*3);
					if (global_ui_context.mouse->scroll_state == SCROLL_DOWN)
						scroll_y-=(h*3);
				}
				
				// scroll with arrow keys
				{
					if (keyboard_is_key_pressed(keyboard, KEY_UP))
						scroll_y+=(h*3);
					if (keyboard_is_key_pressed(keyboard, KEY_DOWN))
						scroll_y-=(h*3);
				}
				
			}
			
			s32 scroll_h = 0;
			s32 scroll_x = window->width - scroll_w;
			
			float scroll_h_percentage = total_h / (float)total_space;
			scroll_h = total_space / scroll_h_percentage;
			
			if (scroll_h < 10)
				scroll_h = 10;
			
			static bool is_scrolling_with_mouse = false;
			if ((mouse->x >= scroll_x && mouse->x < scroll_x + scroll_w &&
				 mouse->y >= start_y && mouse->y < start_y + total_space &&
				 is_left_down(mouse)) || is_scrolling_with_mouse)
			{
				is_scrolling_with_mouse = true;
				
				if (is_left_released(mouse)) is_scrolling_with_mouse = false;
				
				if (main_window->has_focus && mouse->x != MOUSE_OFFSCREEN && mouse->y != MOUSE_OFFSCREEN)
				{
					float new_percentage = (mouse->y - start_y) / (float)total_space;
					scroll_y = -(new_percentage * (total_h-scroll_h));
				}
			}
			
			if (scroll_y > 0)
				scroll_y = 0;
			if (scroll_y < -overflow)
				scroll_y = -overflow;
			
			float percentage = -scroll_y / (float)overflow;
			float scroll_y = start_y + (total_space - scroll_h) * percentage;
			
			// scroll background
			render_rectangle(scroll_x,start_y,
							 scroll_w,total_space,global_ui_context.style.scrollbar_background);
			
			render_rectangle_outline(scroll_x,start_y,
									 scroll_w,total_space, 1,
									 global_ui_context.style.border);
			
			// scrollbar
			render_rectangle(scroll_x,scroll_y,
							 scroll_w,scroll_h,global_ui_context.style.scrollbar_handle_background);
			
			render_rectangle_outline(scroll_x,scroll_y,
									 scroll_w,scroll_h, 1,
									 global_ui_context.style.border);
		}
		////////////////////////
	}
	else
	{
		char *message = localize("no_matches_found");
		s32 w = calculate_text_width(font_small, message);
		s32 x = (window->width / 2) - (w / 2);
		render_text(font_small, x, y, message, global_ui_context.style.foreground);
	}
	
	render_reset_scissor();
}

static void render_info(platform_window *window, font *font_small)
{
	if (!current_search_result->walking_file_system)
	{
		if (current_search_result->show_error_message)
		{
			s32 h = 30;
			s32 yy = global_ui_context.layout.offset_y;
			
			s32 img_size = 16;
			
			for (s32 e = 0; e < current_search_result->errors.length; e++)
			{
				char *message = array_at(&current_search_result->errors, e);
				
				render_image(error_img, 6, yy + (h/2) - (img_size/2), img_size, img_size);
				render_text(font_small, 12 + img_size, yy + (h/2)-(font_small->size/2) + 1, message, ERROR_TEXT_COLOR);
				yy += font_small->size + 4;
			}
			
			yy += font_small->size + 4;
			
			global_ui_context.layout.offset_y = yy;
		}
		
		s32 y = global_ui_context.layout.offset_y;
		
		char *info = localize("info_text");
		
		render_text_cutoff(font_small, 10, y, 
						   info, global_ui_context.style.foreground, window->width - 20);
	}
	else
	{
		// TODO(Aldrik): loading animation here?
	}
}

static s32 prepare_search_directory_path(char *path, s32 len)
{
#ifdef OS_LINUX
	if (path[len-1] != '/' && len < MAX_INPUT_LENGTH)
	{
		path[len] = '/';
		path[len+1] = 0;
		return len+1;
	}
#endif
	
#ifdef OS_WIN
	if ((path[len-1] != '\\' && path[len-1] != '/') && len < MAX_INPUT_LENGTH)
	{
		path[len] = '\\';
		path[len+1] = 0;
		return len+1;
	}
#endif
	
	return len;
}

static void clear_errors()
{
	current_search_result->errors.length = 0;
	current_search_result->show_error_message = false;
}

static void set_error(char *message)
{
	current_search_result->show_error_message = true;
	current_search_result->found_file_matches = false;
	
	array_push(&current_search_result->errors, message);
}

search_result *create_empty_search_result()
{
	search_result *new_result_buffer = mem_alloc(sizeof(search_result));
	new_result_buffer->done_finding_matches = true;
	new_result_buffer->find_duration_us = 0;
	new_result_buffer->show_error_message = false;
	new_result_buffer->found_file_matches = false;
	new_result_buffer->files_searched = 0;
	new_result_buffer->files_matched = 0;
	new_result_buffer->search_result_source_dir_len = 0;
	new_result_buffer->match_found = false;
	new_result_buffer->cancel_search = false;
	new_result_buffer->mutex = mutex_create();
	new_result_buffer->search_id = 0;
	new_result_buffer->done_finding_files = false;
	new_result_buffer->walking_file_system = false;
	
	new_result_buffer->mem_bucket = memory_bucket_init(megabytes(5));
	
	new_result_buffer->errors = array_create(MAX_ERROR_MESSAGE_LENGTH);
	array_reserve(&new_result_buffer->errors, ERROR_RESERVE_COUNT);
	
	// list of files found in current search
	new_result_buffer->files = array_create(sizeof(file_match));
	new_result_buffer->files.reserve_jump = 5000;
	array_reserve(&new_result_buffer->files, FILE_RESERVE_COUNT);
	
	new_result_buffer->matches = array_create(sizeof(file_match));
	new_result_buffer->matches.reserve_jump = 5000;
	array_reserve(&new_result_buffer->matches, FILE_RESERVE_COUNT);
	
	// work queue when searching for matches
	new_result_buffer->work_queue = array_create(sizeof(find_text_args));
	new_result_buffer->work_queue.reserve_jump = 5000;
	array_reserve(&new_result_buffer->work_queue, FILE_RESERVE_COUNT);
	
	new_result_buffer->filter_buffer = textbox_file_filter.buffer;
	new_result_buffer->text_to_find_buffer = textbox_search_text.buffer;
	new_result_buffer->search_directory_buffer = textbox_path.buffer;
	new_result_buffer->recursive_buffer = &checkbox_recursive.state;
	
	return new_result_buffer;
}

static bool start_file_search(search_result *new_result)
{
	bool continue_search = true;
	
	if (continue_search)
	{
		search_result *old_result = current_search_result;
		current_search_result = new_result;
		
		old_result->cancel_search = true;
		thread cleanup_thread = thread_start(destroy_search_result_thread, old_result);
		thread_detach(&cleanup_thread);
		
		new_result->files_searched = 0;
		new_result->cancel_search = false;
		scroll_y = 0;
		new_result->found_file_matches = false;
		new_result->done_finding_files = false;
		
		new_result->max_thread_count = global_settings_page.max_thread_count;
		new_result->max_file_size = global_settings_page.max_file_size;
		
		reset_status_text();
		clear_errors();
		
		// validate input
		{
			if (strcmp(textbox_path.buffer, "") == 0)
			{
				set_error(localize("no_search_directory_specified"));
				continue_search = false;
			}
			
			if (strcmp(textbox_file_filter.buffer, "") == 0)
			{
				set_error(localize("no_file_filter_specified"));
				continue_search = false;
			}
			
			if (strcmp(textbox_search_text.buffer, "") == 0)
			{
				set_error(localize("no_search_text_specified"));
				continue_search = false;
				//strcpy(textbox_search_text.buffer, "*");
			}
			
			if (!platform_directory_exists(textbox_path.buffer))
			{
				set_error(localize("search_directory_does_not_exist"));
				continue_search = false;
			}
		}
		
		if (continue_search)
		{
			new_result->search_id++;
			
			new_result->is_recursive = checkbox_recursive.state;
			new_result->walking_file_system = true;
			new_result->done_finding_matches = false;
			
			new_result->search_result_source_dir_len = strlen(textbox_path.buffer);
			new_result->search_result_source_dir_len = prepare_search_directory_path(textbox_path.buffer, 
																					 new_result->search_result_source_dir_len);
			
			new_result->start_time = platform_get_time(TIME_FULL, TIME_US);
			
			// start search for files
			{
				platform_list_files(&new_result->files, textbox_path.buffer, textbox_file_filter.buffer, checkbox_recursive.state, &new_result->mem_bucket,
									&new_result->cancel_search,
									&new_result->done_finding_files);
			}
		}
	}
	
	return continue_search;
}

static void start_text_search(search_result *new_result)
{
	// start search for text
	char *text_to_find_buf = memory_bucket_reserve(&new_result->mem_bucket, MAX_INPUT_LENGTH);
	strncpy(text_to_find_buf, textbox_search_text.buffer, MAX_INPUT_LENGTH-1);
	new_result->text_to_find = text_to_find_buf;
	find_text_in_files(new_result);
}

static void do_search()
{
	search_result *new_result = create_empty_search_result();
	
	if (start_file_search(new_result))
	{
		set_status_text_to_active();
		start_text_search(new_result);
	}
}

static void load_assets()
{
	search_img = assets_load_image(_binary____data_imgs_search_png_start, 
								   _binary____data_imgs_search_png_end, false);
	logo_small_img = assets_load_image(_binary____data_imgs_logo_64_png_start,
									   _binary____data_imgs_logo_64_png_end, true);
	directory_img = assets_load_image(_binary____data_imgs_folder_png_start,
									  _binary____data_imgs_folder_png_end, false);
	error_img = assets_load_image(_binary____data_imgs_error_png_start,
								  _binary____data_imgs_error_png_end, false);
	
	font_medium = assets_load_font(_binary____data_fonts_mono_ttf_start,
								   _binary____data_fonts_mono_ttf_end, 24);
	font_small = assets_load_font(_binary____data_fonts_mono_ttf_start,
								  _binary____data_fonts_mono_ttf_end, 16);
	font_mini = assets_load_font(_binary____data_fonts_mono_ttf_start,
								 _binary____data_fonts_mono_ttf_end, 12);
	
	// assets used in other window
	font_big = assets_load_font(_binary____data_fonts_mono_ttf_start,
								_binary____data_fonts_mono_ttf_end, 32);
}

void load_config(settings_config *config)
{
	char *path = settings_config_get_string(config, "SEARCH_DIRECTORY");
	bool recursive = settings_config_get_number(config, "SEARCH_DIRECTORIES");
	//bool parallelize = settings_config_get_number(config, "PARALLELIZE_SEARCH");
	char *search_text = settings_config_get_string(config, "SEARCH_TEXT");
	char *search_filter = settings_config_get_string(config, "FILE_FILTER");
	s32 max_thread_count = settings_config_get_number(config, "MAX_THEAD_COUNT");
	s32 max_file_size = settings_config_get_number(config, "MAX_FILE_SIZE");
	char *locale_id = settings_config_get_string(config, "LOCALE");
	s32 window_w = settings_config_get_number(config, "WINDOW_WIDTH");
	s32 window_h = settings_config_get_number(config, "WINDOW_HEIGHT");
	u32 style = settings_config_get_number(config, "STYLE");
	
	if (search_filter)
		strncpy(textbox_file_filter.buffer, search_filter, MAX_INPUT_LENGTH);
	else
		strncpy(textbox_file_filter.buffer, "*.txt,*.c", MAX_INPUT_LENGTH);
	
	if (search_text)
		strncpy(textbox_search_text.buffer, search_text, MAX_INPUT_LENGTH);
	else
		strncpy(textbox_search_text.buffer, "*hello world*", MAX_INPUT_LENGTH);
	
	if (locale_id)
		set_locale(locale_id);
	else
		set_locale("en");
	
	if (style != 0)
	{
		ui_set_style(style);
	}
	if (path)
	{
		strncpy(textbox_path.buffer, path, MAX_INPUT_LENGTH);
		
		checkbox_recursive.state = recursive;
		//global_settings_page.enable_parallelization = parallelize;
		global_settings_page.max_thread_count = max_thread_count;
		global_settings_page.max_file_size = max_file_size;
		global_settings_page.current_style = global_ui_context.style.id;
	}
	else
	{
		checkbox_recursive.state = 1;
		//global_settings_page.enable_parallelization = 1;
		global_settings_page.max_thread_count = 20;
		global_settings_page.max_file_size = 200;
		
		if (is_platform_in_darkmode())
		{
			ui_set_style(UI_STYLE_DARK);
			global_settings_page.current_style = global_ui_context.style.id;
		}
		else
		{
			ui_set_style(UI_STYLE_LIGHT);
			global_settings_page.current_style = global_ui_context.style.id;
		}
		
		strncpy(textbox_path.buffer, DEFAULT_DIRECTORY, MAX_INPUT_LENGTH);
	}
	
	if (window_w >= 800 && window_h >= 600)
        platform_window_set_size(main_window, window_w, window_h);
}

#if defined(OS_LINUX) || defined(OS_WIN)
int main(int argc, char **argv)
{
#if 0
	char *tmp = mem_alloc(100);
	strcpy(tmp, u8"test1ʀ34");
	
	printf("-- %s\n", tmp);
	utf8_str_insert_at(tmp, 1, 52);
	printf("-- %s\n", tmp);
	utf8_str_replace_at(tmp, 2, 40);
	printf("-- %s\n", tmp);
	utf8_str_remove_at(tmp, 3, 40);
	printf("-- %s\n", tmp);
#endif
	
	platform_init(argc, argv);
	
#ifdef MODE_DEVELOPER
	platform_window window = platform_open_window("Text-search [developer]", 800, 600, 0, 0);
#else
	platform_window window = platform_open_window("Text-search", 800, 600, 0, 0);
#endif
	
	main_window = &window;
	
	settings_page_create();
	
	load_available_localizations();
	set_locale("en");
	
	load_assets();
	
	keyboard_input keyboard = keyboard_input_create();
	mouse_input mouse = mouse_input_create();
	
	camera camera;
	camera.x = 0;
	camera.y = 0;
	camera.rotation = 0;
	
	ui_create(&window, &keyboard, &mouse, &camera, font_small);
	
	// asset worker
	thread asset_queue_worker1 = thread_start(assets_queue_worker, NULL);
	thread asset_queue_worker2 = thread_start(assets_queue_worker, NULL);
	thread_detach(&asset_queue_worker1);
	thread_detach(&asset_queue_worker2);
	
	// ui widgets
	checkbox_recursive = ui_create_checkbox(false);
	textbox_search_text = ui_create_textbox(MAX_INPUT_LENGTH);
	textbox_path = ui_create_textbox(MAX_INPUT_LENGTH);
	textbox_file_filter = ui_create_textbox(MAX_INPUT_LENGTH);
	button_select_directory = ui_create_button();
	button_find_text = ui_create_button();
	button_cancel = ui_create_button();
	
	// status bar
	global_status_bar.result_status_text = mem_alloc(MAX_STATUS_TEXT_LENGTH);
	global_status_bar.result_status_text[0] = 0;
	global_status_bar.error_status_text = mem_alloc(MAX_ERROR_MESSAGE_LENGTH);
	global_status_bar.error_status_text[0] = 0;
	
	reset_status_text();
	
	char config_path_buffer[PATH_MAX];
	get_config_save_location(config_path_buffer);
	
	// load config
	settings_config config = settings_config_load_from_file(config_path_buffer);
	load_config(&config);
	
	current_search_result = create_empty_search_result();
	
	while(window.is_open) {
        u64 last_stamp = platform_get_time(TIME_FULL, TIME_US);
		platform_handle_events(&window, &mouse, &keyboard);
		platform_set_cursor(&window, CURSOR_DEFAULT);
		
		settings_page_update_render();
		
		platform_window_make_current(&window);
		
		static bool icon_loaded = false;
		if (!icon_loaded && logo_small_img->loaded)
		{
			icon_loaded = true;
			platform_set_icon(&window, logo_small_img);
		}
		
		static bool assets_loaded = false;
		if (global_asset_collection.queue.queue.length == 0 && !assets_loaded)
		{
			thread_stop(&asset_queue_worker1);
			thread_stop(&asset_queue_worker2);
			
			assets_loaded = true;
		}
		
		global_ui_context.layout.active_window = &window;
		global_ui_context.keyboard = &keyboard;
		global_ui_context.mouse = &mouse;
		
		render_clear();
		camera_apply_transformations(&window, &camera);
		
		global_ui_context.layout.width = global_ui_context.layout.active_window->width;
		// begin ui
		ui_begin(1);
		{
			render_rectangle(0, 0, main_window->width, main_window->height, global_ui_context.style.background);
			
			ui_begin_menu_bar();
			{
				// shortcuts begin
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_O}))
				{
					import_results();
				}
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_S}))
				{
					if (!current_search_result->done_finding_matches)
						platform_show_message(&window, localize("cant_export_when_active"), localize("failed_to_export_results"));
					else if (current_search_result->found_file_matches)
						export_results(current_search_result);
					else
						platform_show_message(&window, localize("no_results_to_export"), localize("failed_to_export_results"));
				}
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_Q}))
				{
					window.is_open = false;
				}
				if (keyboard_is_key_pressed(&keyboard, KEY_ENTER) && !textbox_path.state && !textbox_file_filter.state && !textbox_search_text.state)
				{
					do_search();
				}
				if (keyboard_is_key_pressed(&keyboard, KEY_TAB) && textbox_path.state)
				{
					platform_autocomplete_path(textbox_path.buffer, true);
					keyboard_set_input_text(&keyboard, textbox_path.buffer);
				}
				// shortcuts end
				
				render_rectangle(0, 0, main_window->width, MENU_BAR_HEIGHT, global_ui_context.style.menu_background);
				if (ui_push_menu(localize("file")))
				{
					if (ui_push_menu_item(localize("import"), "Ctrl + O")) 
					{ 
						import_results(); 
					}
					if (ui_push_menu_item(localize("export"), "Ctrl + S")) 
					{ 
						if (!current_search_result->done_finding_matches)
							platform_show_message(&window, localize("cant_export_when_active"), localize("failed_to_export_results"));
						else if (current_search_result->found_file_matches)
							export_results(current_search_result);
						else
							platform_show_message(&window, localize("no_results_to_export"), localize("failed_to_export_results"));
					}
					ui_push_menu_item_separator();
					if (ui_push_menu_item(localize("quit"), "Ctrl + Q")) 
					{ 
						window.is_open = false; 
					}
				}
				if (ui_push_menu(localize("options")))
				{
					if (ui_push_menu_item(localize("settings"), "")) 
					{
						settings_page_show();
					}
				}
			}
			ui_end_menu_bar();
			
			ui_push_separator();
			
			ui_block_begin(LAYOUT_HORIZONTAL);
			{
				if (ui_push_textbox(&textbox_path, localize("search_directory")))
				{
					keyboard_set_input_mode(&global_settings_page.keyboard, INPUT_FULL);
				}
				
				global_ui_context.layout.offset_x -= WIDGET_PADDING - 1;
				if (ui_push_button_image(&button_select_directory, "", directory_img))
				{
					platform_open_file_dialog(OPEN_DIRECTORY, textbox_path.buffer, 0, 0);
				}
				
				if (ui_push_textbox(&textbox_file_filter, localize("file_filter")))
				{
					keyboard_set_input_mode(&global_settings_page.keyboard, INPUT_FULL);
				}
			}
			ui_block_end();
			
			global_ui_context.layout.offset_y -= 5;
			
			ui_block_begin(LAYOUT_HORIZONTAL);
			{
				s32 len1 = strlen(textbox_search_text.buffer);
				if (ui_push_textbox(&textbox_search_text, localize("text_to_find")))
				{
					keyboard_set_input_mode(&global_settings_page.keyboard, INPUT_FULL);
				}
				s32 len2 = strlen(textbox_search_text.buffer);
				if (len1 != len2) do_search();
				
				global_ui_context.layout.offset_x -= WIDGET_PADDING - 1;
				if (ui_push_button_image(&button_find_text, "", search_img))
				{
					do_search();
				}
				ui_push_checkbox(&checkbox_recursive, localize("folders"));
				
				if (!current_search_result->done_finding_matches)
				{
					if (ui_push_button_image(&button_cancel, localize("cancel"), directory_img))
					{
						current_search_result->cancel_search = true;
					}
				}
			}
			ui_block_end();
			
			ui_push_separator();
		}
		ui_end();
		// end ui
		
		// draw info or results
		{
			render_status_bar(&window, font_small);
			
			if (!current_search_result->found_file_matches)
			{
				render_info(&window, font_small);
			}
			else
			{
				render_update_result(&window, font_mini, &mouse, &keyboard);
			}
		}
		
		//render_font_palette(font_mini, -15000, 300, font_mini->palette_width/2, font_mini->palette_height/2, rgb(200,0,0));
		
		assets_do_post_process();
		
		u64 current_stamp = platform_get_time(TIME_FULL, TIME_US);
		u64 diff = current_stamp - last_stamp;
		float diff_ms = diff / 1000.0f;
		last_stamp = current_stamp;
		
		if (diff_ms < TARGET_FRAMERATE)
		{
			double time_to_wait = (TARGET_FRAMERATE) - diff_ms;
			thread_sleep(time_to_wait*1000);
		}
        
		platform_window_swap_buffers(&window);
    }
	
	settings_page_hide_without_save();
	
	// write config file
	settings_config_set_string(&config, "SEARCH_DIRECTORY", textbox_path.buffer);
	settings_config_set_number(&config, "SEARCH_DIRECTORIES", checkbox_recursive.state);
	settings_config_set_string(&config, "SEARCH_TEXT", textbox_search_text.buffer);
	settings_config_set_string(&config, "FILE_FILTER", textbox_file_filter.buffer);
	settings_config_set_number(&config, "MAX_THEAD_COUNT", global_settings_page.max_thread_count);
	settings_config_set_number(&config, "MAX_FILE_SIZE", global_settings_page.max_file_size);
	settings_config_set_number(&config, "WINDOW_WIDTH", window.width);
	settings_config_set_number(&config, "WINDOW_HEIGHT", window.height);
	settings_config_set_number(&config, "STYLE", global_ui_context.style.id);
	
	if (global_localization.active_localization != 0)
	{
		char *current_locale_id = locale_get_id();
		if (current_locale_id)
		{
			settings_config_set_string(&config, "LOCALE", current_locale_id);
		}
	}
	
	settings_config_write_to_file(&config, config_path_buffer);
	settings_config_destroy(&config);
	
	settings_page_destroy();
	
	destroy_available_localizations();
	
	// cleanup ui
	ui_destroy_textbox(&textbox_path);
	ui_destroy_textbox(&textbox_search_text);
	ui_destroy_textbox(&textbox_file_filter);
	ui_destroy();
	mem_free(global_status_bar.result_status_text);
	mem_free(global_status_bar.error_status_text);
	
	// delete assets
	assets_destroy_image(search_img);
	assets_destroy_image(logo_small_img);
	assets_destroy_image(directory_img);
	assets_destroy_image(error_img);
	
	assets_destroy_font(font_small);
	assets_destroy_font(font_mini);
	assets_destroy_font(font_medium);
	assets_destroy_font(font_big);
	
	keyboard_input_destroy(&keyboard);
	platform_destroy_window(&window);
	
	platform_destroy();
	
	return 0;
}
#endif