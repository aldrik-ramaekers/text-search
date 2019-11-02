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

#define TARGET_FRAMERATE 1000/30.0
#define FILE_RESERVE_COUNT 5000
#define ERROR_RESERVE_COUNT 10
#define MAX_ERROR_MESSAGE_LENGTH 70
#define MAX_STATUS_TEXT_LENGTH 100
#define ERROR_TEXT_COLOR rgb(224, 79, 95)

typedef struct t_text_match
{
	found_file file;
	u32 match_count;
	s16 file_error;
	s32 file_size;
	char *line_info;
} text_match;

typedef struct t_status_bar
{
	char *result_status_text;
	char *error_status_text;
} status_bar;

typedef struct t_search_result
{
	array files;
	u64 find_duration_us;
	array errors;
	bool show_error_message; // error occured
	bool found_file_matches; // found/finding matches
	s32 files_searched;
	s32 files_matched;
	s32 search_result_source_dir_len;
	bool match_found;
	mutex mutex;
	bool walking_file_system;
	bool cancel_search;
	bool done_finding_matches;
	char *filter_buffer;
	char *text_to_find_buffer;
	char *search_directory_buffer;
	bool *recursive_state_buffer;
	s32 search_id;
	u64 start_time;
	bool done_finding_files;
	bool is_parallelized;
} search_result;

typedef struct t_find_text_args
{
	text_match *match;
	char *text_to_find;
	s32 search_id;
} find_text_args;

status_bar global_status_bar;
search_result global_search_result;

image *search_img;
image *error_img;
image *directory_img;
image *logo_img;
image *drag_drop_img;
image *notification_bg_img;
image *logo_small_img;
image *gpl_img;

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

// TODO(Aldrik): rewrite ui code
// TODO(Aldrik): open file dialog for folder selection (windows)
// TODO(Aldrik): implement directX11 render layer for windows
// TODO(Aldrik): click on result line to open in active editor (4coder,emacs,vim,gedit,vis studio code)

checkbox_state checkbox_recursive;
textbox_state textbox_search_text;
textbox_state textbox_path;
textbox_state textbox_file_filter;
button_state button_select_directory;
button_state button_find_text;
button_state button_cancel;

static void *find_text_in_file_t(void *arg)
{
	find_text_args *args = arg;
	text_match *match = args->match;
	s32 search_id = args->search_id;
	
	file_content content;
	content.content = 0;
	
	try_again:
	{
		if (global_search_result.cancel_search) { goto finish_early; }
		
		content = platform_read_file_content(match->file.path, "r");
		match->file_size = content.content_length;
		
		s32 kb_to_b = global_settings_page.max_file_size * 1000;
		if (global_settings_page.max_file_size && content.content_length > kb_to_b)
		{
			goto finish_early;
		}
		
		if (global_search_result.cancel_search) { goto finish_early; }
		
		if (content.content && !content.file_error)
		{
			s32 line_nr = 0;
			char *line;
			s32 word_offset = 0;
			if (string_contains_ex(content.content, args->text_to_find, &line_nr, &line, &word_offset))
			{
				global_search_result.match_found = true;
				
				// match info
				match->line_info = memory_bucket_reserve(&global_platform_memory_bucket, word_offset+80); // show 20 chars behind text match. + 10 extra space
				sprintf(match->line_info, "line %d: %.40s", line_nr, word_offset < 20 ? line : line+word_offset-20);
				
				char *tmp = match->line_info;
				while(*tmp)
				{
					if (*tmp == '\n')
					{
						*tmp = 0;
						break;
					}
					++tmp;
				}
				
				if (search_id == global_search_result.search_id)
				{
					mutex_lock(&global_search_result.mutex);
					match->match_count++;
					global_search_result.files_matched++;
					mutex_unlock(&global_search_result.mutex);
				}
			}
		}
		else
		{
			if (content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS ||
				content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM)
			{
				thread_sleep(1000);
				
				goto try_again;
			}
			
			if (content.file_error)
				match->file_error = content.file_error;
			else
				match->file_error = FILE_ERROR_GENERIC;
			
			if (search_id == global_search_result.search_id)
			{
				mutex_lock(&global_search_result.mutex);
				strncpy(global_status_bar.error_status_text, localize("generic_file_open_error"), MAX_ERROR_MESSAGE_LENGTH);
				mutex_unlock(&global_search_result.mutex);
			}
		}
	}
	
	finish_early:
	{
		platform_destroy_file_content(&content);
		
		if (!global_search_result.cancel_search && global_search_result.search_id == search_id)
		{
			mutex_lock(&global_search_result.mutex);
			global_search_result.files_searched++;
			sprintf(global_status_bar.result_status_text, localize("percentage_files_processed"),  (global_search_result.files_searched/(float)global_search_result.files.length)*100);
			mutex_unlock(&global_search_result.mutex);
		}
	}
	
	return 0;
}

static void* find_text_in_files_t(void *arg)
{
	s32 current_search_id = global_search_result.search_id;
	
	array threads = array_create(sizeof(thread));
	strncpy(global_status_bar.error_status_text, "", MAX_ERROR_MESSAGE_LENGTH);
	char *text_to_find = arg;
	
	s32 start = 0;
	s32 len = 0;
	do_work:
	{
		mutex_lock(&global_search_result.files.mutex);
		len = global_search_result.files.length;
		mutex_unlock(&global_search_result.files.mutex);
		
		for (s32 i = start; i < len; i++)
		{
			find_text_args *args = memory_bucket_reserve(&global_platform_memory_bucket, sizeof(find_text_args));
			args->match = array_at(&global_search_result.files, i);
			args->match->match_count = 0;
			args->match->file_error = 0;
			args->match->file_size = 0;
			args->match->line_info = 0;
			args->text_to_find = text_to_find;
			args->search_id = current_search_id;
			
			// limit thread usage
			if (global_settings_page.max_thread_count)
			{
				if (threads.length >= global_settings_page.max_thread_count)
				{
					for (s32 i = 0; i < threads.length; i++)
					{
						thread *thr = array_at(&threads, i);
						bool joined = thread_tryjoin(thr);
						if (joined)
						{
							array_remove(&threads, thr);
							break;
						}
					}
				}
			}
			
			thread new_thr = thread_start(find_text_in_file_t, args);
			
			if (global_search_result.cancel_search) 
			{
				goto finish_early;
			}
			
			if (new_thr.valid)
			{
				array_push(&threads, &new_thr);
				//thread_join(&new_thr);
			}
			else
			{
				i--;
			}
		}
	}
	
	if (global_settings_page.enable_parallelization)
	{
		if (global_search_result.done_finding_files)
		{
			if (len != global_search_result.files.length)
			{
				start = len;
				len = global_search_result.files.length;
				goto do_work;
			}
		}
		else
		{
			start = len;
			len = global_search_result.files.length;
			thread_sleep(1000);
			goto do_work;
		}
		
		global_search_result.done_finding_files = false;
		global_search_result.walking_file_system = false;
	}
	
	for (s32 i = 0; i < threads.length; i++)
	{
		thread *thr = array_at(&threads, i);
		thread_join(thr);
	}
	
	finish_early:
	{
		if (current_search_id == global_search_result.search_id)
		{
			u64 end_f = platform_get_time(TIME_FULL, TIME_US);
			
			global_search_result.find_duration_us = end_f - global_search_result.start_time;
			sprintf(global_status_bar.result_status_text, localize("files_matches_comparison"), global_search_result.files_matched, global_search_result.files.length, global_search_result.find_duration_us/1000.0);
			
			global_search_result.done_finding_matches = true;
			global_search_result.files_searched = global_search_result.files.length;
			
			if (!main_window->has_focus)
				platform_show_alert("Text-search", localize("search_result_completed"));
		}
		array_destroy(&threads);
	}
	
	mem_free(text_to_find);
	
	return 0;
}

static void find_text_in_files(char *text_to_find)
{
	global_search_result.files_matched = 0;
	global_search_result.files_searched = 0;
	global_search_result.found_file_matches = true;
	global_search_result.match_found = false;
	
	thread thr = thread_start(find_text_in_files_t, text_to_find);
	thread_detach(&thr);
}

static void render_drag_drop_feedback(platform_window *window)
{
	if (window->drag_drop_info.state == DRAG_DROP_ENTER)
	{
		char *text = localize("drag_drop_import");
		static s32 rec_width = 450;
		static s32 rec_height = 200;
		static s32 icon_width = 100;
		static s32 icon_height = 100;
		s32 rec_pos_x = (window->width / 2) - (rec_width / 2);
		s32 rec_pos_y = (window->height / 2) - (rec_height / 2);
		s32 icon_pos_x = rec_pos_x + (rec_width / 2) - (icon_width / 2);
		s32 icon_pos_y = rec_pos_y + 30;
		s32 text_w = calculate_text_width(font_small, text);
		s32 text_pos_y = rec_pos_y + rec_height - font_small->size - 35;
		s32 text_pos_x = rec_pos_x + (rec_width / 2) - (text_w / 2);
		
		render_rectangle(0, 0, window->width, window->height, rgba(0,0,0,180));
		render_image_tint(notification_bg_img, rec_pos_x+5, rec_pos_y+5, rec_width, rec_height, rgba(0,0,0, 100));
		render_image(notification_bg_img, rec_pos_x, rec_pos_y, rec_width, rec_height);
		render_image(drag_drop_img, icon_pos_x, icon_pos_y, icon_width, icon_height);
		render_text(font_small, text_pos_x, text_pos_y, text, rgb(35,31,32));
	}
	
	if (window->drag_drop_info.state == DRAG_DROP_FINISHED)
	{
		import_results_from_file(&global_search_result, window->drag_drop_info.path);
	}
}

static void render_status_bar(platform_window *window, font *font_small)
{
	s32 h = 30;
	s32 y = window->height - h;
	s32 img_size = 16;
	
	// result status
	s32 text_size = calculate_text_width(font_small, global_status_bar.result_status_text);
	render_rectangle(-1, y, window->width+2, h, rgb(225,225,225));
	render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
	render_text(font_small, window->width - text_size - 8, y + (h/2)-(font_small->size/2) + 1, global_status_bar.result_status_text, global_ui_context.style.foreground);
	
	
	// error status
	if (global_status_bar.error_status_text[0] != 0)
	{
		render_image(error_img, 6, y + (h/2) - (img_size/2), img_size, img_size);
		render_text(font_small, 12 + img_size, y + (h/2)-(font_small->size/2) + 1, global_status_bar.error_status_text, rgb(224, 79, 95));
	}
}

static void render_update_result(platform_window *window, font *font_small, mouse_input *mouse)
{
	s32 y = global_ui_context.layout.offset_y;
	s32 h = 24;
	
	s32 render_y = y - WIDGET_PADDING;
	s32 render_h;
	
	if (global_settings_page.enable_parallelization)
	{
		render_h = window->height - render_y - 10;
	}
	else
	{
		render_h = window->height - render_y - 30;
	}
	
	render_set_scissor(window, 0, render_y, window->width, render_h);
	
	if (global_search_result.match_found)
	{
		if (!global_settings_page.enable_parallelization)
		{
			render_rectangle(0, y-WIDGET_PADDING, (global_search_result.files_searched/(float)global_search_result.files.length)*window->width, 20, rgb(0,200,0));
			y += 11;
		}
		else
		{
			y -= 9;
		}
		
		s32 path_width = window->width / 2.0;
		s32 pattern_width = window->width / 8.0;
		
		/// header /////////////
		render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
		
		render_rectangle(-1, y+1, window->width+2, h-2, rgb(225,225,225));
		
		render_text(font_small, 10, y + (h/2)-(font_small->size/2) + 1, localize("file_path"), global_ui_context.style.foreground);
		
		render_text(font_small, 10 + path_width, y + (h/2)-(font_small->size/2) + 1, localize("file_pattern"), global_ui_context.style.foreground);
		
		render_text(font_small, 10 + path_width + pattern_width, y + (h/2)-(font_small->size/2) + 1, localize("information"), global_ui_context.style.foreground);
		/////////////////////////
		
		y += h-1;
		
		s32 total_h = 0;
		s32 start_y = y;
		s32 total_space = window->height - start_y - 30 + 1;
		render_set_scissor(window, 0, y, window->width, render_h - 43);
		
		/// draw entries ////////
		s32 drawn_entity_count = 0;
		for (s32 i = 0; i < global_search_result.files.length; i++)
		{
			text_match *match = array_at(&global_search_result.files, i);
			
			if (match->match_count || match->file_error)
			{
				drawn_entity_count++;
				s32 rec_y = y+scroll_y;
				
				if (rec_y > start_y - h && rec_y < start_y + total_space)
				{
#if 0
					// hover item and click item
					if (mouse->y > rec_y && mouse->y < rec_y + h && mouse->y < window->height - 30)
					{
						render_rectangle(-1, rec_y, window->width+2, h, rgb(240,220,220));
						platform_set_cursor(window, CURSOR_POINTER);
					}
#endif
					
					// outline
					render_rectangle_outline(-1, rec_y, window->width+2, h, 1, global_ui_context.style.border);
					
					// path
					render_set_scissor(window, 0, start_y, path_width-10, render_h - 43);
					render_text(font_small, 10, rec_y + (h/2)-(font_small->size/2) + 1, match->file.path + global_search_result.search_result_source_dir_len, global_ui_context.style.foreground);
					
					// pattern
					render_set_scissor(window, 0, start_y, 
									   path_width+pattern_width-10, render_h - 43);
					render_text(font_small, 10 + path_width, rec_y + (h/2)-(font_small->size/2) + 1, match->file.matched_filter, global_ui_context.style.foreground);
					
					// state
					render_set_scissor(window, 0, start_y, window->width, render_h - 43);
					if (!match->file_error)
					{
						//char snum[20];
						//sprintf(snum, "(%d Bytes)", match->file_size);
						if (match->line_info)
							render_text(font_small, 10 + path_width + pattern_width, rec_y + (h/2)-(font_small->size/2) + 1, match->line_info, global_ui_context.style.foreground);
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
				if (global_ui_context.mouse->scroll_state == SCROLL_UP)
					scroll_y+=(h*3);
				if (global_ui_context.mouse->scroll_state == SCROLL_DOWN)
					scroll_y-=70;
			}
			
			if (scroll_y > 0)
				scroll_y = 0;
			if (scroll_y < -overflow)
				scroll_y = -overflow;
			
			s32 scroll_w = 14;
			s32 scroll_h = 0;
			s32 scroll_x = window->width - scroll_w;
			
			float scroll_h_percentage = total_h / (float)total_space;
			scroll_h = total_space / scroll_h_percentage;
			
			if (scroll_h < 10)
				scroll_h = 10;
			
			float percentage = -scroll_y / (float)overflow;
			float scroll_y = start_y + (total_space - scroll_h) * percentage;
			
			// scroll background
			render_rectangle(scroll_x,start_y,
							 scroll_w,total_space,rgb(255,255,255));
			
			render_rectangle_outline(scroll_x,start_y,
									 scroll_w,total_space, 1,
									 global_ui_context.style.border);
			
			// scrollbar
			render_rectangle(scroll_x,scroll_y,
							 scroll_w,scroll_h,rgb(225,225,225));
			
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
	if (!global_search_result.walking_file_system)
	{
		if (global_search_result.show_error_message)
		{
			s32 h = 30;
			s32 yy = global_ui_context.layout.offset_y;
			
			s32 img_size = 16;
			
			for (s32 e = 0; e < global_search_result.errors.length; e++)
			{
				char *message = array_at(&global_search_result.errors, e);
				
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
		s32 img_c = window->width/2;
		s32 img_w = 200;
		s32 img_h = 200;
		s32 img_x = window->width/2-img_w/2;
		s32 img_y = window->height/2-img_h/2-50;
		char text[40];
		
		u64 dot_count_t = platform_get_time(TIME_FULL, TIME_MILI_S);
		s32 dot_count = (dot_count_t % 1000) / 250;
		
		sprintf(text, "%s%.*s", localize("finding_files"), dot_count, "...");
		
		render_image(logo_img, img_x, img_y, img_w, img_h);
		s32 text_w = calculate_text_width(font_medium, text);
		render_text(font_medium, img_c - (text_w/2), img_y + img_h + 50, text, global_ui_context.style.foreground);
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
	global_search_result.errors.length = 0;
	global_search_result.show_error_message = false;
}

static void set_error(char *message)
{
	global_search_result.show_error_message = true;
	global_search_result.found_file_matches = false;
	
	array_push(&global_search_result.errors, message);
}

static void reset_status_text()
{
	strncpy(global_status_bar.result_status_text, localize("no_search_completed"), MAX_STATUS_TEXT_LENGTH);
}

static void do_search()
{
	bool continue_search = true;
	
	if (global_search_result.walking_file_system) continue_search = false;
	if (!global_search_result.done_finding_matches) continue_search = false;
	
	if (continue_search)
	{
		global_search_result.files_searched = 0;
		global_search_result.cancel_search = false;
		scroll_y = 0;
		global_search_result.found_file_matches = false;
		global_search_result.done_finding_files = false;
		global_search_result.is_parallelized = global_settings_page.enable_parallelization;
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
				//set_error(localize("no_search_text_specified"));
				//continue_search = false;
				strcpy(textbox_search_text.buffer, "*");
			}
			
			if (!platform_directory_exists(textbox_path.buffer))
			{
				set_error(localize("search_directory_does_not_exist"));
				continue_search = false;
			}
		}
		
		if (continue_search)
		{
			global_search_result.search_id++;
			
			global_search_result.walking_file_system = true;
			global_search_result.done_finding_matches = false;
			
			global_search_result.search_result_source_dir_len = strlen(textbox_path.buffer);
			global_search_result.search_result_source_dir_len = prepare_search_directory_path(textbox_path.buffer, 
																							  global_search_result.search_result_source_dir_len);
			
			platform_destroy_list_file_result(&global_search_result.files);
			
			global_search_result.start_time = platform_get_time(TIME_FULL, TIME_US);
			platform_list_files(&global_search_result.files, textbox_path.buffer, textbox_file_filter.buffer, checkbox_recursive.state,
								&global_search_result.done_finding_files);
			
			if (global_settings_page.enable_parallelization)
			{
				char *text_to_find_buf = mem_alloc(MAX_INPUT_LENGTH);
				strncpy(text_to_find_buf, textbox_search_text.buffer, MAX_INPUT_LENGTH-1);
				find_text_in_files(text_to_find_buf);
			}
		}
	}
}

static void load_assets()
{
	search_img = assets_load_image("data/imgs/search.png", false);
	logo_img = assets_load_image("data/imgs/text-search-logo_512px.png", false);
	logo_small_img = assets_load_image("data/imgs/text-search-logo_32px.png", true);
	directory_img = assets_load_image("data/imgs/folder.png", false);
	error_img = assets_load_image("data/imgs/error.png", false);
	drag_drop_img = assets_load_image("data/imgs/drag_drop.png", false);
	notification_bg_img = assets_load_image("data/imgs/notification_bg.png", false);
	
	font_medium = assets_load_font("data/fonts/mono.ttf", 24);
	font_small = assets_load_font("data/fonts/mono.ttf", 16);
	font_mini = assets_load_font("data/fonts/mono.ttf", 12);
	
	// assets used in other windo
	gpl_img = assets_load_image("data/imgs/gplv3-or-later.png", false);
	font_big = assets_load_font("data/fonts/mono.ttf", 32);
}

void load_config(settings_config *config)
{
	char *path = settings_config_get_string(config, "SEARCH_DIRECTORY");
	bool recursive = settings_config_get_number(config, "SEARCH_DIRECTORIES");
	bool parallelize = settings_config_get_number(config, "PARALLELIZE_SEARCH");
	char *search_text = settings_config_get_string(config, "SEARCH_TEXT");
	char *search_filter = settings_config_get_string(config, "FILE_FILTER");
	s32 max_thread_count = settings_config_get_number(config, "MAX_THEAD_COUNT");
	s32 max_file_size = settings_config_get_number(config, "MAX_FILE_SIZE");
	char *locale_id = settings_config_get_string(config, "LOCALE");
	s32 window_w = settings_config_get_number(config, "WINDOW_WIDTH");
	s32 window_h = settings_config_get_number(config, "WINDOW_HEIGHT");
	
	if (path)
		strncpy(textbox_path.buffer, path, MAX_INPUT_LENGTH);
	else
		strncpy(textbox_path.buffer, "/home/", MAX_INPUT_LENGTH);
	
	if (search_filter)
		strncpy(textbox_file_filter.buffer, search_filter, MAX_INPUT_LENGTH);
	else
		strncpy(textbox_file_filter.buffer, "*.txt,*.c", MAX_INPUT_LENGTH);
	
	if (search_text)
		strncpy(textbox_search_text.buffer, search_text, MAX_INPUT_LENGTH);
	else
		strncpy(textbox_search_text.buffer, "*hello world*", MAX_INPUT_LENGTH);
	
	checkbox_recursive.state = recursive;
	global_settings_page.enable_parallelization = parallelize;
	global_settings_page.max_thread_count = max_thread_count;
	global_settings_page.max_file_size = max_file_size;
	
	if (locale_id)
		set_locale(locale_id);
	else
		set_locale("en");
	
	if (window_w >= 800 && window_h >= 600)
        platform_window_set_size(main_window, window_w, window_h);
}

#if defined(OS_LINUX) || defined(OS_WIN)
int main(int argc, char **argv)
{
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
	
	// starting values
	global_search_result.done_finding_matches = true;
	global_search_result.find_duration_us = 0;
	global_search_result.show_error_message = false;
	global_search_result.found_file_matches = false;
	global_search_result.files_searched = 0;
	global_search_result.files_matched = 0;
	global_search_result.search_result_source_dir_len = 0;
	global_search_result.match_found = false;
	global_search_result.cancel_search = false;
	global_search_result.mutex = mutex_create();
	global_search_result.search_id = 0;
	global_search_result.done_finding_files = false;
	
	global_search_result.errors = array_create(MAX_ERROR_MESSAGE_LENGTH);
	array_reserve(&global_search_result.errors, ERROR_RESERVE_COUNT);
	
	reset_status_text();
	
	global_search_result.files = array_create(sizeof(text_match));
	// alocate space for 1000 extra files when limit is reached
	global_search_result.files.reserve_jump = 1000;
	array_reserve(&global_search_result.files, FILE_RESERVE_COUNT);
	
	// load config
	settings_config config = settings_config_load_from_file("data/config.txt");
	load_config(&config);
	
	global_search_result.filter_buffer = textbox_file_filter.buffer;
	global_search_result.text_to_find_buffer = textbox_search_text.buffer;
	global_search_result.search_directory_buffer = textbox_path.buffer;
	global_search_result.recursive_state_buffer = &checkbox_recursive.state;
	
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
			global_ui_context.style.background_hover = rgb(190,190,190);
			global_ui_context.style.background = rgb(225,225,225);
			global_ui_context.style.border = rgb(180,180,180);
			global_ui_context.style.foreground = rgb(10, 10, 10);
			global_ui_context.style.textbox_background = rgb(240,240,240);
			global_ui_context.style.textbox_foreground = rgb(10,10,10);
			global_ui_context.style.textbox_active_border = rgb(66, 134, 244);
			global_ui_context.style.button_background = rgb(225,225,225);
			
			ui_begin_menu_bar();
			{
				// shortcuts begin
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_O}))
				{
					import_results(&global_search_result);
				}
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_S}))
				{
					if (global_search_result.found_file_matches)
						export_results(&global_search_result);
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
				if (keyboard_is_key_pressed(&keyboard, KEY_TAB))
				{
					platform_autocomplete_path(textbox_path.buffer, true);
					keyboard_set_input_text(&keyboard, textbox_path.buffer);
				}
				// shortcuts end
				
				if (ui_push_menu(localize("file")))
				{
					if (ui_push_menu_item(localize("import"), "Ctrl + O")) 
					{ 
						import_results(&global_search_result); 
					}
					if (ui_push_menu_item(localize("export"), "Ctrl + S")) 
					{ 
						if (global_search_result.found_file_matches)
							export_results(&global_search_result); 
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
			
			global_ui_context.style.background = rgb(255,255,255);
			
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
				if (ui_push_textbox(&textbox_search_text, localize("text_to_find")))
				{
					keyboard_set_input_mode(&global_settings_page.keyboard, INPUT_FULL);
				}
				
				global_ui_context.layout.offset_x -= WIDGET_PADDING - 1;
				if (ui_push_button_image(&button_find_text, "", search_img))
				{
					do_search();
				}
				ui_push_checkbox(&checkbox_recursive, localize("folders"));
				
				if (global_search_result.walking_file_system || !global_search_result.done_finding_matches)
				{
					if (ui_push_button_image(&button_cancel, localize("cancel"), directory_img))
					{
						platform_cancel_search = true;
						global_search_result.cancel_search = true;
						global_search_result.done_finding_matches = true;
						global_search_result.walking_file_system = false;
					}
				}
			}
			ui_block_end();
			
			ui_push_separator();
		}
		ui_end();
		// end ui
		
		if (!global_settings_page.enable_parallelization)
		{
			if (global_search_result.done_finding_files)
			{
				char *text_to_find_buf = mem_alloc(MAX_INPUT_LENGTH);
				strncpy(text_to_find_buf, textbox_search_text.buffer, MAX_INPUT_LENGTH-1);
				
				find_text_in_files(text_to_find_buf);
				global_search_result.done_finding_files = false;
				global_search_result.walking_file_system = false;
			}
		}
		
		// draw info or results
		{
			render_status_bar(&window, font_small);
			
			if (!global_search_result.found_file_matches)
			{
				render_info(&window, font_small);
			}
			else
			{
				render_update_result(&window, font_mini, &mouse);
			}
		}
		
		render_drag_drop_feedback(&window);
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
	settings_config_set_number(&config, "PARALLELIZE_SEARCH", global_settings_page.enable_parallelization);
	
	if (global_localization.active_localization != 0)
	{
		char *current_locale_id = localize_get_id();
		if (current_locale_id)
		{
			settings_config_set_string(&config, "LOCALE", current_locale_id);
		}
	}
	
	settings_config_write_to_file(&config, "data/config.txt");
#ifdef MODE_DEVELOPER
	settings_config_write_to_file(&config, "../data/config.txt");
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
	
	// cleanup resuls
	mutex_destroy(&global_search_result.mutex);
	array_destroy(&global_search_result.files);
	array_destroy(&global_search_result.errors);
	
	// delete assets
	assets_destroy_image(search_img);
	assets_destroy_image(logo_img);
	assets_destroy_image(logo_small_img);
	assets_destroy_image(directory_img);
	assets_destroy_image(error_img);
	assets_destroy_image(drag_drop_img);
	assets_destroy_image(notification_bg_img);
	assets_destroy_image(gpl_img);
	
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