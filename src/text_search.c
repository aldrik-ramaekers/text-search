/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#include "config.h"
#include "asset_definitions.h"
#include "../../project-base/src/project_base.h"

// TODO(Aldrik): sorting by clicking on tab
// TODO(Aldrik): scrolling a textbox and then making window bigger stops textbox from scrolling back

typedef struct t_status_bar
{
	char *result_status_text;
	char *error_status_text;
} status_bar;

status_bar global_status_bar;
search_result *current_search_result;
s32 next_search_id = 0;

image *search_img;
image *error_img;
image *directory_img;
image *logo_small_img;

font *font_small;
font *font_mini;
s32 scroll_y = 0;

///// our global ui states ////
checkbox_state checkbox_recursive;
textbox_state textbox_search_text;
textbox_state textbox_path;
textbox_state textbox_file_filter;
button_state button_select_directory;
button_state button_find_text;
button_state button_cancel;
///////////////////////////////

#include "save.h"
#include "settings.h"
#include "command_line.h"

#include "save.c"
#include "settings.c"
#include "command_line.c"

void* destroy_search_result_thread(void *arg)
{
	search_result *buffer = arg;
	
	while(!buffer->threads_closed || !buffer->done_finding_files)
	{
		thread_sleep(1000); // 1ms
	}
	
	array_destroy(&buffer->matches);
	array_destroy(&buffer->errors);
	array_destroy(&buffer->files);
	
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
			
			mutex_lock(&result_buffer->mutex);
			result_buffer->files_searched++;
			mutex_unlock(&result_buffer->mutex);
			
			retry_search:;
			
			// read file size
			s32 file_size = platform_get_file_size(args.file.file.path);
			s32 kb_to_b = result_buffer->max_file_size * 1000;
			
			// check if file is not too big for set filter
			if (result_buffer->max_file_size > 0 && (file_size > kb_to_b || file_size == -1))
			{
				args.file.file_size = file_size;
				file_match file_match = args.file;
				file_match.file_error = FILE_ERROR_TOO_BIG;
				file_match.line_nr = 0;
				file_match.line_info = 0;
				file_match.word_match_offset = 0;
				file_match.word_match_length = 0;
				file_match.word_match_offset_x = 0;
				file_match.word_match_width = 0;
				args.search_result_buffer->match_found = true;
				array_push(&result_buffer->matches, &file_match);
				
				continue;
			}
			
			// read file
			content = platform_read_file_content(args.file.file.path, "rb");
			args.file.file_size = content.content_length;
			
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
						file_match.line_info = memory_bucket_reserve(&result_buffer->mem_bucket, 80*4+10);
						
						s32 offset_to_render = m->word_offset;
						
#define MAX_CHARS_LEFT 15
						s32 overflow = 0;
						if (offset_to_render > MAX_CHARS_LEFT)
						{
							overflow = (offset_to_render-MAX_CHARS_LEFT);
							offset_to_render = offset_to_render - overflow;
						}
						
						char *str_to_copy = utf8_str_upto(m->line_start, overflow);
						
						snprintf(file_match.line_info, 80*4+10, "%.80s", str_to_copy);
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
						
						file_match.word_match_offset = offset_to_render;
						file_match.word_match_length = m->word_match_len;
						
						if (!result_buffer->is_command_line_search)
						{
							file_match.word_match_offset_x = 
								calculate_text_width_upto(font_mini, file_match.line_info, offset_to_render);
							
							file_match.word_match_width =
								calculate_text_width_from_upto(font_mini, file_match.line_info, offset_to_render, offset_to_render + m->word_match_len);
						}
						else
						{
							printf("%s:%d:%s\n", file_match.file.path,
								   file_match.line_nr, file_match.file.matched_filter);
						}
						
						array_push(&result_buffer->matches, &file_match);
						
						mutex_lock(&result_buffer->mutex);
						result_buffer->match_count++;
						mutex_unlock(&result_buffer->mutex);
						
						main_window->do_draw = true;
					}
				}
				
				array_destroy(&text_matches);
			}
			else
			{
				if (content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS ||
					content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM)
				{
					thread_sleep(1000);
					goto retry_search;
				}
				
				if (content.file_error)
					args.file.file_error = content.file_error;
				else
					args.file.file_error = FILE_ERROR_GENERIC;
				printf("%d %d %p\n", args.file.file_error, content.file_error, content.content);
				mutex_lock(&args.search_result_buffer->mutex);
				string_copyn(global_status_bar.error_status_text, localize("generic_file_open_error"), MAX_ERROR_MESSAGE_LENGTH);
				mutex_unlock(&args.search_result_buffer->mutex);
			}
			
			platform_destroy_file_content(&content);
		}
		else
		{
			mutex_unlock(&result_buffer->work_queue.mutex);
		}
	}
	
	if (!result_buffer->cancel_search && !result_buffer->is_command_line_search)
	{
		mutex_lock(&result_buffer->mutex);
		snprintf(global_status_bar.result_status_text, MAX_INPUT_LENGTH, localize("percentage_files_processed"),  (result_buffer->files_searched/(float)result_buffer->files.length)*100);
		mutex_unlock(&result_buffer->mutex);
	}
	
	finish_early:;
	return 0;
}

static void* find_text_in_files_t(void *arg)
{
	search_result *result_buffer = arg;
	
	array threads = array_create(sizeof(thread));
	array_reserve(&threads, result_buffer->max_thread_count);
	
	if (!result_buffer->is_command_line_search)
		string_copyn(global_status_bar.error_status_text, "", MAX_ERROR_MESSAGE_LENGTH);
	char *text_to_find = result_buffer->text_to_find;
	
	// create worker threads
	if (result_buffer->max_thread_count <= 0)
		result_buffer->max_thread_count = DEFAULT_THREAD_COUNT;
	
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
	
	finish_early:
	{
		u64 end_f = platform_get_time(TIME_FULL, TIME_US);
		
		result_buffer->find_duration_us = end_f - result_buffer->start_time;
		result_buffer->done_finding_matches = true;
		result_buffer->walking_file_system = false;
		
		if (!result_buffer->is_command_line_search && !main_window->has_focus)
			platform_show_alert("Text-search", localize("search_result_completed"));
		
	}
	
	for (s32 i = 0; i < threads.length; i++)
	{
		thread *thr = array_at(&threads, i);
		thread_join(thr);
	}
	result_buffer->files_searched = result_buffer->files.length;
	
	if (!result_buffer->is_command_line_search)
	{
		snprintf(global_status_bar.result_status_text, MAX_INPUT_LENGTH, localize("files_matches_comparison"), result_buffer->match_count, result_buffer->files_searched, result_buffer->find_duration_us/1000.0);
	}
	
	array_destroy(&threads);
	array_destroy(&result_buffer->work_queue);
	
	result_buffer->threads_closed = true;
	
	return 0;
}

void find_text_in_files(search_result *search_result)
{
	search_result->files_matched = 0;
	search_result->files_searched = 0;
	search_result->found_file_matches = true;
	search_result->match_found = false;
	
	thread thr1 = thread_start(find_text_in_files_t, search_result);
	thread_detach(&thr1);
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
	render_text(font_small, window->width - text_size - 8, y + (h/2)-(font_small->px_h/2), global_status_bar.result_status_text, global_ui_context.style.foreground);
	render_reset_scissor(main_window);
	
	// error status
	if (global_status_bar.error_status_text[0] != 0)
	{
		render_set_scissor(main_window, 0, y, main_window->width/2, h);
		render_image(error_img, 6, y + (h/2) - (img_size/2), img_size, img_size);
		render_text(font_small, 12 + img_size, y + (h/2)-(font_small->px_h/2), global_status_bar.error_status_text, global_ui_context.style.error_foreground);
		render_reset_scissor(main_window);
	}
	else
	{
		char buffer[100];
		
		render_set_scissor(main_window, 0, y, main_window->width/2, h);
		snprintf(buffer, 100, "%"PRId64" %s %"PRId64" %s",
				 current_search_result->search_info.dir_count,
				 localize("files_handled_1"),
				 current_search_result->search_info.file_count,
				 localize("files_handled_2"));
		
		render_text(font_small, 5, y + (h/2)-(font_small->px_h/2), buffer, global_ui_context.style.foreground);
		
		render_reset_scissor(main_window);
	}
}

void set_status_text_to_finished_search()
{
	main_window->do_draw = true;
	
	snprintf(global_status_bar.result_status_text, MAX_INPUT_LENGTH, localize("files_matches_comparison"), current_search_result->matches.length, current_search_result->files.length, current_search_result->find_duration_us/1000.0);
}

static void set_status_text_to_active()
{
	main_window->do_draw = true;
	
	char text[40];
	
	u64 dot_count_t = platform_get_time(TIME_FULL, TIME_MILI_S);
	s32 dot_count = (dot_count_t % 1000) / 250;
	
	snprintf(text, 40, "%.*s%s", dot_count, "...", localize("finding_files"));
	
	string_copyn(global_status_bar.result_status_text, text, MAX_STATUS_TEXT_LENGTH);
}

static void set_status_text_to_cancelled()
{
	main_window->do_draw = true;
	
	string_copyn(global_status_bar.result_status_text, localize("cancelling_search"), MAX_STATUS_TEXT_LENGTH);
}

void reset_status_text()
{
	main_window->do_draw = true;
	
	string_copyn(global_status_bar.result_status_text, localize("no_search_completed"), MAX_STATUS_TEXT_LENGTH);
}

static s32 path_width = 300;
static s32 pattern_width = 150;

static bool dragging_path = false;
static bool dragging_pattern = false;

static bool render_update_result_header_entry(s32 x, s32 y, s32 w, platform_window *window, font *font_small, mouse_input *mouse, bool dragging, bool can_drag, s32 *ww, char *text)
{
	s32 h = 24;
	
	if (mouse->x > x+w-5 && mouse->x < x+w+5 && mouse->y >= y && mouse->y <= y+h && can_drag)
	{
		platform_set_cursor(window, CURSOR_DRAG);
		if (is_left_down(mouse))
		{
			dragging = true;
		}
	}
	if (dragging)
	{
		platform_set_cursor(window, CURSOR_DRAG);
		
		*ww = mouse->x - x;
		
		if (is_left_released(mouse))
		{
			dragging = false;
		}
	}
	if (path_width + pattern_width > window->width - 100)
		path_width = window->width - pattern_width - 100;
	if (path_width < 100)
		path_width = 100;
	if (pattern_width + path_width > window->width - 100)
		pattern_width = window->width - path_width - 100;
	if (pattern_width < 105)
		pattern_width = 105;
	
	// bg
	render_rectangle(x, y+1, *ww+1, h-2, global_ui_context.style.info_bar_background);
	render_rectangle(x, y+h-1, *ww+1, 1, global_ui_context.style.border);
	
	// splitter + text
	render_rectangle(x, y+1, 1, h-2, global_ui_context.style.border);
	render_text(font_small, x + 10, y + (h/2)-(font_small->px_h/2), text, global_ui_context.style.foreground);
	
	return dragging;
}

static s32 render_update_result_header(platform_window *window, font *font_small, mouse_input *mouse, keyboard_input *keyboard)
{
	s32 y = global_ui_context.layout.offset_y - 9;
	
#if 1
	dragging_path = render_update_result_header_entry(-1, y, path_width, window, 
													  font_small, mouse, dragging_path, !dragging_pattern, &path_width, localize("file_path"));
	
	dragging_pattern = render_update_result_header_entry(path_width, y, pattern_width, 
														 window, font_small, mouse, 
														 dragging_pattern, !dragging_path, &pattern_width, localize("file_pattern"));
	
	s32 information_width = window->width - path_width - pattern_width;
	render_update_result_header_entry(path_width + pattern_width, y, 
									  information_width, window, 
									  font_small, mouse, false, false, &information_width, localize("information"));
#else
	s32 h = 24;
	
	// path width
	if (mouse->x > path_width-5 && mouse->x < path_width+5 && mouse->y >= y && mouse->y <= y+h && !dragging_pattern)
	{
		platform_set_cursor(window, CURSOR_DRAG);
		if (is_left_down(mouse))
		{
			dragging_path = true;
		}
	}
	if (dragging_path)
	{
		platform_set_cursor(window, CURSOR_DRAG);
		
		path_width = mouse->x;
		
		if (is_left_released(mouse))
		{
			dragging_path = false;
		}
	}
	if (path_width + pattern_width > window->width - 100)
		path_width = window->width - pattern_width - 100;
	if (path_width < 100)
		path_width = 100;
	
	// pattern width
	if (mouse->x > path_width+pattern_width-5 && mouse->x < path_width+pattern_width+5 && mouse->y >= y && mouse->y <= y+h && !dragging_path)
	{
		platform_set_cursor(window, CURSOR_DRAG);
		if (is_left_down(mouse))
		{
			dragging_pattern = true;
		}
	}
	if (dragging_pattern)
	{
		platform_set_cursor(window, CURSOR_DRAG);
		
		pattern_width = mouse->x - path_width;
		
		if (is_left_released(mouse))
		{
			dragging_pattern = false;
		}
	}
	if (pattern_width + path_width > window->width - 100)
		pattern_width = window->width - path_width - 100;
	if (pattern_width < 105)
		pattern_width = 105;
	
	render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
	
	render_rectangle(-1, y+1, window->width+2, h-2, global_ui_context.style.info_bar_background);
	
	render_text(font_small, 10, y + (h/2)-(font_small->px_h/2), localize("file_path"), global_ui_context.style.foreground);
	
	render_rectangle(path_width, y+1, 1, h-2, global_ui_context.style.border);
	
	render_text(font_small, 10 + path_width, y + (h/2)-(font_small->px_h/2), localize("file_pattern"), global_ui_context.style.foreground);
	
	render_rectangle(path_width + pattern_width, y+1, 1, h-2, global_ui_context.style.border);
	
	render_text(font_small, 10 + path_width + pattern_width, y + (h/2)-(font_small->px_h/2), localize("information"), global_ui_context.style.foreground);
	
#endif
	
	return y;
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
	s32 draw_h = 24;
	s32 logical_h = 23;
	
	s32 render_y = y - WIDGET_PADDING;
	s32 render_h;
	render_h = window->height - render_y - 10;
	
	render_set_scissor(window, 0, render_y, window->width, render_h);
	
	y = render_update_result_header(window, font_small, mouse, keyboard);
	
	if (current_search_result->match_found)
	{
		y += logical_h;
		
		s32 scroll_w = 14;
		s32 total_h = 0;
		s32 start_y = y;
		s32 total_space = window->height - start_y - 30 + 1;
		render_set_scissor(window, 0, y, window->width, render_h - 43);
		
		// calculate what to draw
		s32 start_iter = -scroll_y / (logical_h);
		y += (logical_h)*start_iter;
		s32 max_len = (total_space / draw_h)+2;
		
		/// draw entries ////////
		for (s32 i = start_iter; i < start_iter + max_len; i++)
		{
			if (i >= current_search_result->matches.length) continue;
			
			file_match *match = array_at(&current_search_result->matches, i);
			
			s32 rec_y = y+scroll_y;
			
			if (rec_y > start_y - draw_h && rec_y < start_y + total_space)
			{
				// hover item and click item
				if (mouse->y > rec_y && mouse->y < rec_y + draw_h && mouse->y < window->height - 30 &&
					mouse->x >= 0 && mouse->x < window->width - scroll_w)
				{
					if (is_left_double_clicked(mouse))
					{
						push_notification(localize("copied_to_clipboard"));
						if (match->file_error)
						{
							platform_set_clipboard(main_window, match->file.path);
						}
						else
						{
							switch(global_settings_page.selected_double_click_selection_option)
							{
								case OPTION_PATH: 
								platform_set_clipboard(main_window, match->file.path);
								break;
								
								case OPTION_PATH_LINE:
								{
									char *clipboard_tmp_buffer = malloc(200);
									snprintf(clipboard_tmp_buffer, 200, "%s:%d", match->file.path, match->line_nr);
									platform_set_clipboard(main_window, clipboard_tmp_buffer);
									mem_free(clipboard_tmp_buffer);
								}
								break;
								
								case OPTION_PATH_LINE_FILTER:
								{
									char *clipboard_tmp_buffer = malloc(200 + MAX_INPUT_LENGTH);
									snprintf(clipboard_tmp_buffer, 200 + MAX_INPUT_LENGTH, "%s:%d:%s", match->file.path, match->line_nr, match->file.matched_filter);
									platform_set_clipboard(main_window, clipboard_tmp_buffer);
									mem_free(clipboard_tmp_buffer);
								}
								break;
								
								case OPTION_RESULT: 
								platform_set_clipboard(main_window, match->line_info);
								break;
							}
						}
					}
					
					render_rectangle(-1, rec_y, window->width+2, draw_h, global_ui_context.style.item_hover_background);
					platform_set_cursor(window, CURSOR_POINTER);
				}
				
				// outline
				render_rectangle_outline(-1, rec_y, window->width+2, draw_h, 1, global_ui_context.style.border);
				
				// path
				render_set_scissor(window, 0, start_y, path_width-10, render_h - 43);
				render_text(font_small, 10, rec_y + (draw_h/2)-(font_small->px_h/2), match->file.path + current_search_result->search_result_source_dir_len, global_ui_context.style.foreground);
				
				// pattern
				render_set_scissor(window, 0, start_y, 
								   path_width+pattern_width-10, render_h - 43);
				render_text(font_small, 10 + path_width, rec_y + (draw_h/2)-(font_small->px_h/2), match->file.matched_filter, global_ui_context.style.foreground);
				
				// state
				render_set_scissor(window, 0, start_y, window->width, render_h - 43);
				if (!match->file_error && match->line_info)
				{
					s32 text_sx = 10 + path_width + pattern_width;
					s32 text_sy = rec_y + (draw_h/2)-(font_small->px_h/2);
					
					char tmp[80];
					snprintf(tmp, 80, "line %d: ", match->line_nr);
					
					text_sx += render_text(font_small, text_sx, text_sy, 
										   tmp, global_ui_context.style.foreground);
					
					// highlight matched text
					render_rectangle(text_sx+match->word_match_offset_x, text_sy-1, match->word_match_width, font_small->px_h+2, rgba(255,0,0,80));
					
					render_text(font_small, text_sx, text_sy, 
								match->line_info, global_ui_context.style.foreground);
				}
				else
				{
					s32 img_size = 14;
					render_image(error_img, 6 + path_width + pattern_width, rec_y + (draw_h/2) - (img_size/2), img_size, img_size);
					
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
						case FILE_ERROR_TOO_BIG: open_file_error = localize("file_too_big"); break;
					}
					
					render_text(font_small, 10 + path_width + pattern_width + img_size + 6, rec_y + (draw_h/2)-(font_small->px_h/2), open_file_error, global_ui_context.style.error_foreground);
				}
			}
			y += logical_h;
		}
		////////////////////////
		
		total_h += ((logical_h)*current_search_result->matches.length);
		
		s32 overflow = total_h - total_space;
		
		/// scrollbar //////////
		if (overflow > 0)
		{
			if (global_ui_context.mouse->y >= start_y && global_ui_context.mouse->y <= start_y + total_space)
			{
				// scroll with mouse
				{
					if (global_ui_context.mouse->scroll_state == SCROLL_UP)
						scroll_y+=(draw_h*3);
					if (global_ui_context.mouse->scroll_state == SCROLL_DOWN)
						scroll_y-=(draw_h*3);
				}
				
				// scroll with arrow keys
				{
					if (keyboard_is_key_pressed(keyboard, KEY_UP))
						scroll_y+=(draw_h*3);
					if (keyboard_is_key_pressed(keyboard, KEY_DOWN))
						scroll_y-=(draw_h*3);
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
				 is_left_down(mouse) && !dragging_pattern && !dragging_path) || is_scrolling_with_mouse)
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
		y += 30;
		
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
				render_text(font_small, 12 + img_size, yy + (h/2)-(font_small->px_h/2), message, global_ui_context.style.error_foreground);
				yy += 20;
			}
			
			yy += 20;
			
			global_ui_context.layout.offset_y = yy;
		}
		
		s32 y = global_ui_context.layout.offset_y;
		
		char *info = localize("info_text");
		
		render_text_cutoff(font_small, 10, y, 
						   info, global_ui_context.style.foreground, window->width - 20);
	}
}

s32 prepare_search_directory_path(char *path, s32 len)
{
	// add slash to end of path to make make a valid directory path
	
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
	new_result_buffer->search_id = next_search_id++;
	new_result_buffer->done_finding_files = false;
	new_result_buffer->walking_file_system = false;
	new_result_buffer->is_command_line_search = false;
	new_result_buffer->threads_closed = false;
	new_result_buffer->search_info.dir_count = 0;
	new_result_buffer->search_info.file_count = 0;
	new_result_buffer->match_count = 0;
	
	new_result_buffer->mem_bucket = memory_bucket_init(megabytes(5));
	
	new_result_buffer->errors = array_create(MAX_ERROR_MESSAGE_LENGTH);
	array_reserve(&new_result_buffer->errors, ERROR_RESERVE_COUNT);
	
	// list of files found in current search
	new_result_buffer->files = array_create(sizeof(file_match));
	new_result_buffer->files.reserve_jump = FILE_RESERVE_COUNT;
	array_reserve(&new_result_buffer->files, FILE_RESERVE_COUNT);
	
	new_result_buffer->matches = array_create(sizeof(file_match));
	new_result_buffer->matches.reserve_jump = FILE_RESERVE_COUNT;
	array_reserve(&new_result_buffer->matches, FILE_RESERVE_COUNT);
	
	// work queue when searching for matches
	new_result_buffer->work_queue = array_create(sizeof(find_text_args));
	new_result_buffer->work_queue.reserve_jump = FILE_RESERVE_COUNT;
	array_reserve(&new_result_buffer->work_queue, FILE_RESERVE_COUNT);
	
	// filter buffers
	new_result_buffer->text_to_find = memory_bucket_reserve(&new_result_buffer->mem_bucket, MAX_INPUT_LENGTH);
	new_result_buffer->directory_to_search = memory_bucket_reserve(&new_result_buffer->mem_bucket, MAX_INPUT_LENGTH);
	new_result_buffer->file_filter = memory_bucket_reserve(&new_result_buffer->mem_bucket, MAX_INPUT_LENGTH);
	new_result_buffer->export_path = memory_bucket_reserve(&new_result_buffer->mem_bucket, MAX_INPUT_LENGTH);
	new_result_buffer->export_path[0] = 0;
	
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
		
		scroll_y = 0;
		
		new_result->max_thread_count = global_settings_page.max_thread_count;
		new_result->max_file_size = global_settings_page.max_file_size;
		
		reset_status_text();
		clear_errors();
		
		// validate input
		{
			if (string_equals(textbox_path.buffer, ""))
			{
				set_error(localize("no_search_directory_specified"));
				continue_search = false;
			}
			
			if (string_equals(textbox_file_filter.buffer, ""))
			{
				set_error(localize("no_file_filter_specified"));
				continue_search = false;
			}
			
			if (string_equals(textbox_search_text.buffer, ""))
			{
				set_error(localize("no_search_text_specified"));
				continue_search = false;
			}
			
			if (!platform_directory_exists(textbox_path.buffer))
			{
				set_error(localize("search_directory_does_not_exist"));
				continue_search = false;
			}
		}
		
		if (continue_search)
		{
			string_copyn(new_result->directory_to_search, textbox_path.buffer, MAX_INPUT_LENGTH);
			string_copyn(new_result->file_filter, textbox_file_filter.buffer, MAX_INPUT_LENGTH);
			
			new_result->is_recursive = checkbox_recursive.state;
			new_result->walking_file_system = true;
			new_result->done_finding_matches = false;
			
			new_result->search_result_source_dir_len = strlen(new_result->directory_to_search);
			new_result->search_result_source_dir_len = prepare_search_directory_path(new_result->directory_to_search, 
																					 new_result->search_result_source_dir_len);
			
			new_result->start_time = platform_get_time(TIME_FULL, TIME_US);
			
			// start search for files
			{
				platform_list_files(&new_result->files, new_result->directory_to_search, new_result->file_filter, new_result->is_recursive, &new_result->mem_bucket,
									&new_result->cancel_search,
									&new_result->done_finding_files,
									&new_result->search_info);
			}
		}
	}
	
	return continue_search;
}

static void start_text_search(search_result *new_result)
{
	// start search for text
	string_copyn(new_result->text_to_find, textbox_search_text.buffer, MAX_INPUT_LENGTH);
	find_text_in_files(new_result);
}

void do_search()
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
	search_img = load_bitmap(search_bmp);
	logo_small_img = load_bitmap(logo_64_bmp);
	directory_img = load_bitmap(folder_bmp);
	error_img = load_bitmap(error_bmp);
	
	font_small = load_font(mono_ttf, 15);
	font_mini = load_font(mono_ttf, 12);
}

void load_config(settings_config *config)
{
	char *path = settings_config_get_string(config, "SEARCH_DIRECTORY");
	bool recursive = settings_config_get_number(config, "SEARCH_DIRECTORIES");
	char *search_text = settings_config_get_string(config, "SEARCH_TEXT");
	char *search_filter = settings_config_get_string(config, "FILE_FILTER");
	s32 max_thread_count = settings_config_get_number(config, "MAX_THEAD_COUNT");
	s32 max_file_size = settings_config_get_number(config, "MAX_FILE_SIZE");
	char *locale_id = settings_config_get_string(config, "LOCALE");
	u32 style = settings_config_get_number(config, "STYLE");
	u32 double_click_action = settings_config_get_number(config, "DOUBLE_CLICK_ACTION");
	
	s32 tab1 = settings_config_get_number(config, "TAB_PATH");
	s32 tab2 = settings_config_get_number(config, "TAB_PATTERN");
	
	if (tab1 != 0) path_width = tab1;
	if (tab2 != 0) pattern_width = tab2;
	if (path_width < 100)
		path_width = 100;
	if (pattern_width < 105)
		pattern_width = 105;
	
	if (search_filter)
		string_copyn(textbox_file_filter.buffer, search_filter, MAX_INPUT_LENGTH);
	else
		string_copyn(textbox_file_filter.buffer, "*.txt", MAX_INPUT_LENGTH);
	
	if (search_text)
		string_copyn(textbox_search_text.buffer, search_text, MAX_INPUT_LENGTH);
	else
		string_copyn(textbox_search_text.buffer, "*hello world*", MAX_INPUT_LENGTH);
	
	if (locale_id)
		set_locale(locale_id);
	else
		set_locale("en");
	
	if (path)
	{
		string_copyn(textbox_path.buffer, path, MAX_INPUT_LENGTH);
		
		checkbox_recursive.state = recursive;
		global_settings_page.max_thread_count = max_thread_count;
		global_settings_page.max_file_size = max_file_size;
		global_settings_page.current_style = global_ui_context.style.id;
		global_settings_page.selected_double_click_selection_option = double_click_action;
		
		if (global_settings_page.max_thread_count <= 0)
			global_settings_page.max_thread_count = DEFAULT_THREAD_COUNT;
		
		if (global_settings_page.max_file_size > 999999999)
			global_settings_page.max_file_size = DEFAULT_MAX_FILE_SIZE;
	}
	else
	{
		checkbox_recursive.state = DEFAULT_RECURSIVE_STATE;
		global_settings_page.max_thread_count = DEFAULT_THREAD_COUNT;
		global_settings_page.max_file_size = DEFAULT_MAX_FILE_SIZE;
		global_settings_page.current_style = DEFAULT_STYLE;
		global_settings_page.selected_double_click_selection_option = OPTION_PATH;
		
#if defined(OS_WIN)
		string_copyn(textbox_path.buffer, DEFAULT_WINDOWS_DIRECTORY, MAX_INPUT_LENGTH);
#elif defined(OS_LINUX)
		string_copyn(textbox_path.buffer, DEFAULT_LINUX_DIRECTORY, MAX_INPUT_LENGTH);
#endif
	}
}

#ifdef MODE_TEST
#include "tests.c"
#endif

#if defined(OS_LINUX) || defined(OS_WIN)
int main(int argc, char **argv)
{
	debug_print_elapsed_title("program setup");
	debug_print_elapsed_indent();
	
	platform_init(argc, argv);
	
#ifdef MODE_DEVELOPER
	u64 startup_stamp = platform_get_time(TIME_FULL, TIME_US);
	u64 _startup_stamp = startup_stamp;
#endif
	
	bool is_command_line_run = (argc > 1);
	if (is_command_line_run)
	{
		handle_command_line_arguments(argc, argv);
		return 0;
	}
	
	char config_path_buffer[PATH_MAX];
	get_config_save_location(config_path_buffer);
	
	// load config
	settings_config config = settings_config_load_from_file(config_path_buffer);
	
	s32 window_w = settings_config_get_number(&config, "WINDOW_WIDTH");
	s32 window_h = settings_config_get_number(&config, "WINDOW_HEIGHT");
	if (window_w < 800 || window_h < 600)
	{
		window_w = 800;
		window_h = 600;
	}
	global_use_gpu = settings_config_get_number_or_default(&config, "USE_GPU", 1);
	debug_print_elapsed(startup_stamp, "config");
	
#ifdef MODE_TEST
	window_w = 800;
	window_h = 600;
#endif
	
	platform_window window = platform_open_window("Text-search", window_w, window_h, 0, 0, 800, 600);
	main_window = &window;
	
#ifdef MODE_DEVELOPER
	startup_stamp = platform_get_time(TIME_FULL, TIME_US);
#endif
	
	settings_page_create();
	
	debug_print_elapsed(startup_stamp, "settings page");
	
	// asset workers
	thread asset_queue_worker1 = thread_start(assets_queue_worker, NULL);
	thread asset_queue_worker2 = thread_start(assets_queue_worker, NULL);
	thread asset_queue_worker3 = thread_start(assets_queue_worker, NULL);
	thread_detach(&asset_queue_worker1);
	thread_detach(&asset_queue_worker2);
	thread_detach(&asset_queue_worker3);
	
	debug_print_elapsed(startup_stamp, "asset workers");
	
	load_available_localizations();
	set_locale("en");
	
	debug_print_elapsed(startup_stamp, "locale");
	
	load_assets();
	debug_print_elapsed(startup_stamp, "assets");
	
	keyboard_input keyboard = keyboard_input_create();
	mouse_input mouse = mouse_input_create();
	
#ifdef MODE_TEST
	run_tests(&window, &keyboard, &mouse);
#endif
	
	camera camera;
	camera.x = 0;
	camera.y = 0;
	camera.rotation = 0;
	
	ui_create(&window, &keyboard, &mouse, &camera, font_small);
	
	debug_print_elapsed(startup_stamp, "ui");
	
	// ui widgets
	checkbox_recursive = ui_create_checkbox(false);
	textbox_search_text = ui_create_textbox(MAX_INPUT_LENGTH);
	textbox_search_text.deselect_on_enter = false;
	textbox_path = ui_create_textbox(MAX_INPUT_LENGTH);
	textbox_file_filter = ui_create_textbox(MAX_INPUT_LENGTH);
	button_select_directory = ui_create_button();
	button_find_text = ui_create_button();
	button_cancel = ui_create_button();
	debug_print_elapsed(startup_stamp, "ui widgets");
	
	load_config(&config);
	debug_print_elapsed(startup_stamp, "apply config");
	
	// status bar
	global_status_bar.result_status_text = mem_alloc(MAX_STATUS_TEXT_LENGTH);
	global_status_bar.result_status_text[0] = 0;
	global_status_bar.error_status_text = mem_alloc(MAX_ERROR_MESSAGE_LENGTH);
	global_status_bar.error_status_text[0] = 0;
	reset_status_text();
	debug_print_elapsed(startup_stamp, "status bar");
	
	current_search_result = create_empty_search_result();
	debug_print_elapsed(startup_stamp, "setup");
	
	debug_print_elapsed_undent();
	debug_print_elapsed(_startup_stamp, "total startup time");
	
	while(window.is_open) {
		u64 last_stamp = platform_get_time(TIME_FULL, TIME_US);
		platform_handle_events(&window, &mouse, &keyboard);
		platform_set_cursor(&window, CURSOR_DEFAULT);
		
		settings_page_update_render();
		platform_window_make_current(&window);
		platform_set_icon(&window, logo_small_img);
		
		global_ui_context.layout.active_window = &window;
		global_ui_context.keyboard = &keyboard;
		global_ui_context.mouse = &mouse;
		
		if (assets_do_post_process())
			window.do_draw = true;
		
		if (window.has_focus)
			window.do_draw = true;
		
		if (window.do_draw)
		{
#ifdef CHECKFPS
			u64 tmp1  = platform_get_time(TIME_FULL, TIME_US);
			static s32 total = 0;
			static s32 min = 0;
			static s32 max = 0;
			static s32 count = 0;
#endif
			
			window.do_draw = false;
			
			render_clear(&window);
			camera_apply_transformations(&window, &camera);
			
			global_ui_context.layout.width = global_ui_context.layout.active_window->width;
			// begin ui
			
			ui_begin(1);
			{
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
					if (keyboard_is_key_pressed(&keyboard, KEY_ENTER) && !textbox_path.state && !textbox_file_filter.state)
					{
						do_search();
					}
					if (keyboard_is_key_pressed(&keyboard, KEY_TAB) && textbox_path.state)
					{
						platform_autocomplete_path(textbox_path.buffer, true);
						ui_set_textbox_text(&textbox_path, textbox_path.buffer);
					}
					// shortcuts end
					
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
						ui_set_textbox_text(&textbox_path, textbox_path.buffer);
					}
					ui_push_tooltip(localize("tooltip_browse_directories"));
					
					if (ui_push_textbox(&textbox_file_filter, localize("file_filter")))
					{
						keyboard_set_input_mode(&global_settings_page.keyboard, INPUT_FULL);
					}
				}
				ui_block_end();
				
				global_ui_context.layout.offset_y -= 5;
				
				ui_block_begin(LAYOUT_HORIZONTAL);
				{
					if (ui_push_textbox(&textbox_search_text, localize("text_to_find")))
					{
						keyboard_set_input_mode(&global_settings_page.keyboard, INPUT_FULL);
						if (keyboard.text_changed && string_length(textbox_search_text.buffer) >= 3) do_search();
					}
					
					global_ui_context.layout.offset_x -= WIDGET_PADDING - 1;
					if (ui_push_button_image(&button_find_text, "", search_img))
					{
						do_search();
					}
					ui_push_tooltip(localize("tooltip_start_searching"));
					
					ui_push_checkbox(&checkbox_recursive, localize("folders"));
					ui_push_tooltip(localize("tooltip_search_folders_recursively"));
					
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
			
			update_render_notifications();
			
			platform_window_swap_buffers(&window);
			
#ifdef CHECKFPS
			{
				count++;
				u64 tmp2 = platform_get_time(TIME_FULL, TIME_US);
				u64 tmp3 = tmp2 - tmp1;
				if (tmp3 < max || max == 0) max = tmp3;
				if (tmp3 > min || min == 0) min = tmp3;
				total += tmp3;
				
				if (count % 24 == 0)
					printf("avg: %.2f, min: %.2f, max: %.2f\n", 1000000.0f/(total/count), 1000000.0f/min, 1000000.0f/max);
			}
#endif
		}
		
		u64 current_stamp = platform_get_time(TIME_FULL, TIME_US);
		u64 diff = current_stamp - last_stamp;
		float diff_ms = diff / 1000.0f;
		last_stamp = current_stamp;
		
		if (diff_ms < TARGET_FRAMERATE)
		{
			double time_to_wait = (TARGET_FRAMERATE) - diff_ms;
			thread_sleep(time_to_wait*1000);
		}
	}
	
	settings_page_hide_without_save();
	
	// write config file
	settings_config_set_string(&config, "SEARCH_DIRECTORY", textbox_path.buffer);
	settings_config_set_number(&config, "SEARCH_DIRECTORIES", checkbox_recursive.state);
	settings_config_set_string(&config, "SEARCH_TEXT", textbox_search_text.buffer);
	settings_config_set_string(&config, "FILE_FILTER", textbox_file_filter.buffer);
	settings_config_set_number(&config, "MAX_THEAD_COUNT", global_settings_page.max_thread_count);
	settings_config_set_number(&config, "MAX_FILE_SIZE", global_settings_page.max_file_size);
	settings_config_set_number(&config, "TAB_PATH", path_width);
	settings_config_set_number(&config, "TAB_PATTERN", pattern_width);
	
	vec2 win_size = platform_get_window_size(&window);
	settings_config_set_number(&config, "WINDOW_WIDTH", win_size.x);
	settings_config_set_number(&config, "WINDOW_HEIGHT", win_size.y);
	
	settings_config_set_number(&config, "STYLE", global_ui_context.style.id);
	settings_config_set_number(&config, "DOUBLE_CLICK_ACTION", global_settings_page.selected_double_click_selection_option);
	settings_config_set_number(&config, "USE_GPU", global_use_gpu);
	
	if (global_localization.active_localization != 0)
	{
		char *current_locale_id = locale_get_id();
		if (current_locale_id)
		{
			settings_config_set_string(&config, "LOCALE", current_locale_id);
		}
	}
	
#ifndef MODE_TEST
	settings_config_write_to_file(&config, config_path_buffer);
#endif
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
	assets_destroy_bitmap(search_img);
	assets_destroy_bitmap(logo_small_img);
	assets_destroy_bitmap(directory_img);
	assets_destroy_bitmap(error_img);
	
	assets_destroy_font(font_small);
	assets_destroy_font(font_mini);
	
	keyboard_input_destroy(&keyboard);
	platform_destroy_window(&window);
	
	platform_destroy();
	
	return 0;
}
#endif