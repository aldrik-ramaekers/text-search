#include "config.h"
#include "project_base.h"

#define MATCH_RESERVE_COUNT 5000
#define ERROR_RESERVE_COUNT 20
#define MAX_ERROR_MESSAGE_LENGTH 70
#define MAX_STATUS_TEXT_LENGTH 100
#define ERROR_TEXT_COLOR rgb(224, 79, 95)
#define COMPANY_NAME "Aldrik Ramaekers"
#define WEBSITE_URL "https://google.com"
#define WEBSITE_CONTACT_URL "https://google.com"
#define WEBSITE_MANUAL_URL "https://google.com"

typedef struct t_text_match
{
	found_file file;
	u32 match_count;
	s16 file_error;
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
} search_result;

status_bar global_status_bar;
search_result global_search_result;

image *search_img;
image *error_img;
image *directory_img;
image *sloth_img;
font *font_medium;
font *font_small;
font *font_mini;
image *sloth_small_img;
s32 scroll_y = 0;

#include "save.h"
#include "about.h"

#include "save.c"
#include "about.c"

// TODO(Aldrik): percentage processed goes over 100% when previous is cancelled
// TODO(Aldrik): save filter,path,text on exit and load on start
// TODO(Aldrik): filter on file formats in import/export
// TODO(Aldrik): rename platform_open_file_dialog_d to platform_open_file_dialog_block
// TODO(Aldrik): refactor globals into structs
// TODO(Aldrik): localization.
// TODO(Aldrik): if we want to limit thread count we could use pthread_tryjoin_np

char *text_to_find;

static void *find_text_in_file_t(void *arg)
{
	text_match *match = arg;
	if (global_search_result.cancel_search) return 0;
	
	file_content content;
	try_again:
	{
		if (global_search_result.cancel_search) return 0;
		
		content = platform_read_file_content(match->file.path, "r");
		
		if (content.content && !content.file_error)
		{
			if (global_search_result.cancel_search) return 0;
			
			if (string_contains(content.content, text_to_find))
			{
				global_search_result.match_found = true;
				
				mutex_lock(&global_search_result.mutex);
				match->match_count++;
				global_search_result.files_matched++;
				mutex_unlock(&global_search_result.mutex);
			}
		}
		else
		{
			if (content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS ||
				content.file_error == FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM)
			{
				// TODO(Aldrik): is this really necessary? were already hogging the entire system anyway...
				thread_sleep(1000);
				
				goto try_again;
			}
			
			if (content.file_error)
				match->file_error = content.file_error;
			else
				match->file_error = FILE_ERROR_GENERIC;
			
			mutex_lock(&global_search_result.mutex);
			strcpy(global_status_bar.error_status_text, "One or more files could not be opened");
			mutex_unlock(&global_search_result.mutex);
		}
	}
	
	platform_destroy_file_content(&content);
	
	mutex_lock(&global_search_result.mutex);
	global_search_result.files_searched++;
	sprintf(global_status_bar.result_status_text, "%.0f%% of files processed",  (global_search_result.files_searched/(float)global_search_result.files.length)*100);
	mutex_unlock(&global_search_result.mutex);
	
	return 0;
}

static void* find_text_in_files_t(void *arg)
{
	u64 start_f = platform_get_time(TIME_FULL, TIME_US);
	array threads = array_create(sizeof(thread));
	strcpy(global_status_bar.error_status_text, "");
	
	text_to_find = arg;
	for (s32 i = 0; i < global_search_result.files.length; i++)
	{
		text_match *match = array_at(&global_search_result.files, i);
		match->match_count = 0;
		match->file_error = 0;
		
		thread new_thr = thread_start(find_text_in_file_t, match);
		
		if (global_search_result.cancel_search) break;
		
		if (new_thr.valid)
		{
			array_push(&threads, &new_thr);
		}
		else
		{
			i--;
		}
	}
	
	for (s32 i = 0; i < threads.length; i++)
	{
		thread *thr = array_at(&threads, i);
		thread_join(thr);
	}
	
	u64 end_f = platform_get_time(TIME_FULL, TIME_US);
	
	global_search_result.find_duration_us = end_f - start_f;
	sprintf(global_status_bar.result_status_text, "%d out of %d files matched in %.2fms", global_search_result.files_matched, global_search_result.files.length, global_search_result.find_duration_us/1000.0);
	
	array_destroy(&threads);
	global_search_result.done_finding_matches = true;
	
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

static void render_status_bar(platform_window *window, font *font_small)
{
	s32 h = 30;
	s32 y = window->height - h;
	s32 img_size = 16;
	
	// result status
	s32 text_size = calculate_text_width(font_small, global_status_bar.result_status_text);
	render_rectangle(-1, y, window->width-2, h, rgb(225,225,225));
	render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
	render_text(font_small, window->width - text_size - 8, y + (h/2)-(font_small->size/2) + 1, global_status_bar.result_status_text, global_ui_context.style.foreground);
	
	
	// error status
	if (global_status_bar.error_status_text[0] != 0)
	{
		render_image(error_img, 6, y + (h/2) - (img_size/2), img_size, img_size);
		render_text(font_small, 12 + img_size, y + (h/2)-(font_small->size/2) + 1, global_status_bar.error_status_text, rgb(224, 79, 95));
	}
}

static void render_result(platform_window *window, font *font_small)
{
	s32 y = global_ui_context.layout.offset_y;
	s32 h = 24;
	
	s32 render_y = y - WIDGET_PADDING;
	s32 render_h = window->height - 30 - render_y; // TODO(Aldrik): make constants.. 30 is status bar height
	render_set_scissor(window, 0, render_y, window->width, render_h);
	
	if (global_search_result.match_found)
	{
		render_rectangle(0, y-WIDGET_PADDING, (global_search_result.files_searched/(float)global_search_result.files.length)*window->width, 20, rgb(0,200,0));
		y += 11;
		
		s32 path_width = window->width / 1.8;
		s32 pattern_width = window->width / 6;
		
		/// header /////////////
		render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
		
		render_rectangle(-1, y+1, window->width+2, h-2, rgb(225,225,225));
		
		render_text(font_small, 10, y + (h/2)-(font_small->size/2) + 1, "File path", global_ui_context.style.foreground);
		
		render_text(font_small, 10 + path_width, y + (h/2)-(font_small->size/2) + 1, "File pattern", global_ui_context.style.foreground);
		
		render_text(font_small, 10 + path_width + pattern_width, y + (h/2)-(font_small->size/2) + 1, "Information", global_ui_context.style.foreground);
		/////////////////////////
		
		y += h-1;
		
		s32 total_h = 0;
		s32 start_y = y;
		s32 total_space = window->height - start_y - 30 + 1;
		render_set_scissor(window, 0, y, window->width, render_h - 43);
		
		/// draw entries ////////
		for (s32 i = 0; i < global_search_result.files.length; i++)
		{
			text_match *match = array_at(&global_search_result.files, i);
			
			
			if (match->match_count || match->file_error)
			{
				s32 text_y = y+scroll_y;
				
				if (text_y > start_y - h && text_y < start_y + total_space)
				{
					// outline
					render_rectangle_outline(-1, text_y, window->width+2, h, 1, global_ui_context.style.border);
					
					// path
					render_set_scissor(window, 0, start_y, path_width-10, render_h - 43);
					render_text(font_small, 10, text_y + (h/2)-(font_small->size/2) + 1, match->file.path + global_search_result.search_result_source_dir_len, global_ui_context.style.foreground);
					
					// pattern
					render_set_scissor(window, 0, start_y, 
									   path_width+pattern_width-10, render_h - 43);
					render_text(font_small, 10 + path_width, text_y + (h/2)-(font_small->size/2) + 1, match->file.matched_filter, global_ui_context.style.foreground);
					
					// state
					render_set_scissor(window, 0, start_y, window->width, render_h - 43);
					if (!match->file_error)
					{
						char snum[10];
						sprintf(snum, "%d", match->match_count);
						
						render_text(font_small, 10 + path_width + pattern_width, text_y + (h/2)-(font_small->size/2) + 1, snum, global_ui_context.style.foreground);
					}
					else
					{
						s32 img_size = 14;
						render_image(error_img, 6 + path_width + pattern_width, text_y + (h/2) - (img_size/2), img_size, img_size);
						
						char *open_file_error = 0;
						switch(match->file_error)
						{
							case FILE_ERROR_NO_ACCESS: open_file_error = "No permission"; break;
							case FILE_ERROR_NOT_FOUND: open_file_error = "Not found"; break;
							case FILE_ERROR_CONNECTION_ABORTED: open_file_error = "Connection aborted"; break;
							case FILE_ERROR_CONNECTION_REFUSED: open_file_error = "Connection refused"; break;
							case FILE_ERROR_NETWORK_DOWN: open_file_error = "Network down"; break;
							case FILE_ERROR_REMOTE_IO_ERROR: open_file_error = "Remote error"; break;
							case FILE_ERROR_STALE: open_file_error = "Remotely removed"; break;
							case FILE_ERROR_GENERIC: open_file_error = "Failed to open file"; break;
						}
						
						render_text(font_small, 10 + path_width + pattern_width + img_size + 6, text_y + (h/2)-(font_small->size/2) + 1, open_file_error, ERROR_TEXT_COLOR);
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
			if (global_ui_context.mouse->scroll_state == SCROLL_UP)
				scroll_y+=(h*3);
			if (global_ui_context.mouse->scroll_state == SCROLL_DOWN)
				scroll_y-=70;
			
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
		char *message = "no matches found";
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
			
			global_ui_context.layout.offset_y = yy;
		}
		
		s32 y = global_ui_context.layout.offset_y;
		
		s32 directory_info_count = 11;
		char *info_text[] = 
		{
			"1. Search directory",
			" - The absolute path to the folder that should be searched through for text matches.", 
			"2. File filter",
			" - Filter files that should be included in the search.",
			" - Multiple filters can be split using ','.",
			" - Supports wildcard '*' in filter. (e.g. \"*.jpg,*.png\")",
			"3. Text filter",
			" - Filter on text within the filtered files.",
			" - Supports wildcard '*' in filter. (e.g. \"my name is *\")",
			"4. Folders",
			" - Specifies whether or not folders will be recursively searched for file matches."
		};
		
		// draw info text
		for (s32 i = 0; i < directory_info_count; i++)
		{
			y += render_text_cutoff(font_small, 10, y, 
									info_text[i], global_ui_context.style.foreground, window->width - 20);
		}
	}
	else
	{
		s32 img_c = window->width/2;
		s32 img_w = 200;
		s32 img_h = 200;
		s32 img_x = window->width/2-img_w/2;
		s32 img_y = window->height/2-img_h/2-50;
		char text[20];
		
		u64 dot_count_t = platform_get_time(TIME_FULL, TIME_MS);
		s32 dot_count = (dot_count_t % 1000) / 250;
		
		sprintf(text, "%s%.*s", "Finding files", dot_count, "...");
		
		render_image(sloth_img, img_x, img_y, img_w, img_h);
		s32 text_w = calculate_text_width(font_medium, text);
		render_text(font_medium, img_c - (text_w/2), img_y + img_h + 50, text, global_ui_context.style.foreground);
	}
}

static s32 prepare_search_directory_path(char *path, s32 len)
{
	if (path[len-1] != '/' && len < MAX_INPUT_LENGTH)
	{
		path[len] = '/';
		path[len+1] = 0;
		return len+1;
	}
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
	strcpy(global_status_bar.result_status_text, "No search completed.");
}

int main(int argc, char **argv)
{
	platform_init();
	
	platform_window window = platform_open_window("Text-search", 800, 600);
	
	assets_create();
	audio_system_create();
	about_page_create();
	
#ifdef MODE_DEVELOPER
	info_menu_create();
#endif
	
	search_img = assets_load_image("data/imgs/search.png", false);
	sloth_img = assets_load_image("data/imgs/sloth.png", false);
	sloth_small_img = assets_load_image("data/imgs/sloth_small.png", true);
	directory_img = assets_load_image("data/imgs/folder.png", false);
	error_img = assets_load_image("data/imgs/error.png", false);
	
	font_medium = assets_load_font("data/fonts/mono.ttf", 24);
	font_small = assets_load_font("data/fonts/mono.ttf", 16);
	font_mini = assets_load_font("data/fonts/mono.ttf", 12);
	
	keyboard_input keyboard = keyboard_input_create();
	mouse_input mouse = mouse_input_create();
	
	camera camera;
	camera.x = 0;
	camera.y = 0;
	camera.rotation = 0;
	
	ui_create(&window, &keyboard, &mouse, &camera, font_small);
	
	// asset worker
	thread asset_queue_worker1 = thread_start(assets_queue_worker, NULL);
	thread_detach(&asset_queue_worker1);
	
	// ui widgets
	checkbox_state checkbox_recursive = ui_create_checkbox(false);
	textbox_state textbox_search_text = ui_create_textbox(MAX_INPUT_LENGTH);
	textbox_state textbox_path = ui_create_textbox(MAX_INPUT_LENGTH);
	textbox_state textbox_file_filter = ui_create_textbox(MAX_INPUT_LENGTH);
	button_state button_select_directory = ui_create_button();
	button_state button_find_text = ui_create_button();
	button_state button_cancel = ui_create_button();
	
	// status bar
	global_status_bar.result_status_text = malloc(MAX_STATUS_TEXT_LENGTH);
	global_status_bar.result_status_text[0] = 0;
	global_status_bar.error_status_text = malloc(MAX_STATUS_TEXT_LENGTH);
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
	
	global_search_result.errors = array_create(MAX_ERROR_MESSAGE_LENGTH);
	array_reserve(&global_search_result.errors, MATCH_RESERVE_COUNT);
	
	reset_status_text();
	
	global_search_result.files = array_create(sizeof(text_match));
	array_reserve(&global_search_result.files, MATCH_RESERVE_COUNT);
	
#ifdef MODE_DEVELOPER
	//strcpy(textbox_path.buffer, "/home/aldrik/Projects/text-search/src");
	//strcpy(textbox_file_filter.buffer, "*.c,*.h");
	//strcpy(textbox_search_text.buffer, "*hello*");
	
	strcpy(textbox_path.buffer, "/home/aldrik/Projects/text-search/");
	strcpy(textbox_file_filter.buffer, "*");
	strcpy(textbox_search_text.buffer, "*test*");
	checkbox_recursive.state = 1;
#endif
	
	bool done_finding_files = false;
	
	while(window.is_open) {
		platform_handle_events(&window, &mouse, &keyboard);
		about_page_update_render();
		platform_window_make_current(&window);
		
		static bool loaded = false;
		if (!loaded && sloth_small_img->loaded)
		{
			loaded = true;
			platform_set_icon(&window, sloth_small_img);
		}
		
		global_ui_context.keyboard = &keyboard;
		global_ui_context.mouse = &mouse;
		
		glClearColor(255/255.0, 255/255.0, 255/255.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		camera_apply_transformations(&window, &camera);
		
		// begin ui
		ui_begin();
		{
			global_ui_context.style.background_hover = rgb(190,190,190);
			global_ui_context.style.background = rgb(225,225,225);
			global_ui_context.style.border = rgb(180,180,180);
			global_ui_context.style.foreground = rgb(10, 10, 10);
			global_ui_context.style.textbox_background = rgb(240,240,240);
			global_ui_context.style.textbox_foreground = rgb(10,10,10);
			global_ui_context.style.textbox_active_border = rgb(0,0,200);
			global_ui_context.style.button_background = rgb(225,225,225);
			
			ui_begin_menu_bar();
			{
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_O}))
				{
					import_results(&global_search_result);
				}
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_S}))
				{
					if (global_search_result.found_file_matches)
						export_results(&global_search_result);
				}
				if (is_shortcut_down((s32[2]){KEY_LEFT_CONTROL,KEY_Q}))
				{
					window.is_open = false;
				}
				
				if (ui_push_menu("File"))
				{
					if (ui_push_menu_item("Import", "Ctrl + O")) 
					{ 
						import_results(&global_search_result); 
					}
					if (ui_push_menu_item("Export", "Ctrl + S")) 
					{ 
						if (global_search_result.found_file_matches)
							export_results(&global_search_result); 
					}
					ui_push_menu_item_separator();
					if (ui_push_menu_item("Quit", "Ctrl + Q")) 
					{ 
						window.is_open = false; 
					}
				}
				if (ui_push_menu("Options"))
				{
					if (ui_push_menu_item("Edit", "Ctrl + E")) { }
				}
				if (ui_push_menu("Help"))
				{
					if (ui_push_menu_item("User Manual", ""))
					{
						platform_open_url(WEBSITE_MANUAL_URL);
					}
					if (ui_push_menu_item("About", "")) 
					{
						// TODO(Aldrik): about page
						about_page_show();
					}
					ui_push_menu_item_separator();
					if (ui_push_menu_item("Contact", "")) 
					{
						platform_open_url(WEBSITE_CONTACT_URL);
					}
				}
			}
			ui_end_menu_bar();
			
			global_ui_context.style.background = rgb(255,255,255);
			
			ui_push_separator();
			
			ui_block_begin(LAYOUT_HORIZONTAL);
			{
				ui_push_textbox(&textbox_path, "Search directory");
				global_ui_context.layout.offset_x -= WIDGET_PADDING - 1;
				if (ui_push_button_image(&button_select_directory, "", directory_img))
				{
					platform_open_file_dialog(OPEN_DIRECTORY, textbox_path.buffer);
				}
				
				ui_push_textbox(&textbox_file_filter, "File filter...");
			}
			ui_block_end();
			
			global_ui_context.layout.offset_y -= 5;
			
			ui_block_begin(LAYOUT_HORIZONTAL);
			{
				ui_push_textbox(&textbox_search_text, "Text to find..");
				global_ui_context.layout.offset_x -= WIDGET_PADDING - 1;
				if (ui_push_button_image(&button_find_text, "", search_img))
				{
					global_search_result.files_searched = 0;
					global_search_result.cancel_search = false;
					scroll_y = 0;
					global_search_result.found_file_matches = false;
					done_finding_files = false;
					reset_status_text();
					clear_errors();
					
					bool continue_search = true;
					if (strcmp(textbox_path.buffer, "") == 0)
					{
						set_error("No search directory specified.");
						continue_search = false;
					}
					
					if (strcmp(textbox_file_filter.buffer, "") == 0)
					{
						set_error("No file filter specified.");
						continue_search = false;
					}
					
					if (strcmp(textbox_search_text.buffer, "") == 0)
					{
						set_error("No search text specified.");
						continue_search = false;
					}
					
					if (continue_search)
					{
						global_search_result.walking_file_system = true;
						global_search_result.done_finding_matches = false;
						
						global_search_result.search_result_source_dir_len = strlen(textbox_path.buffer);
						global_search_result.search_result_source_dir_len = prepare_search_directory_path(textbox_path.buffer, 
																										  global_search_result.search_result_source_dir_len);
						
						for (s32 i = 0; i < global_search_result.files.length; i++)
						{
							text_match *match = array_at(&global_search_result.files, i);
							free(match->file.path);
							free(match->file.matched_filter);
						}
						global_search_result.files.length = 0;
						
						u64 start_f = platform_get_time(TIME_FULL, TIME_US);
						platform_list_files(&global_search_result.files, textbox_path.buffer, textbox_file_filter.buffer, checkbox_recursive.state,
											&done_finding_files);
					}
				}
				ui_push_checkbox(&checkbox_recursive, "Folders");
				
				if (global_search_result.walking_file_system || !global_search_result.done_finding_matches)
				{
					if (ui_push_button_image(&button_cancel, "Cancel", directory_img))
					{
						platform_cancel_search = true;
						global_search_result.cancel_search = true;
						global_search_result.done_finding_matches = true;
					}
				}
			}
			ui_block_end();
			
			ui_push_separator();
		}
		ui_end();
		// end ui
		
		if (done_finding_files)
		{
			find_text_in_files(textbox_search_text.buffer);
			done_finding_files = false;
			global_search_result.walking_file_system = false;
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
				render_result(&window, font_mini);
			}
		}
		
		
		assets_do_post_process();
		
		platform_window_swap_buffers(&window);
	}
	
	thread_stop(&asset_queue_worker1);
	
#ifdef MODE_DEVELOPER
	info_menu_destroy();
#endif
	
	about_page_destroy();
	ui_destroy();
	
	mutex_destroy(&global_search_result.mutex);
	array_destroy(&global_search_result.files);
	array_destroy(&global_search_result.errors);
	free(global_status_bar.result_status_text);
	free(global_status_bar.error_status_text);
	
	assets_destroy_image(search_img);
	assets_destroy_image(sloth_img);
	assets_destroy_image(sloth_small_img);
	assets_destroy_image(directory_img);
	assets_destroy_image(error_img);
	
	assets_destroy_font(font_small);
	assets_destroy_font(font_mini);
	assets_destroy_font(font_medium);
	assets_destroy();
	audio_system_destroy();
	
	keyboard_input_destroy(&keyboard);
	platform_close_window(&window);
	platform_destroy_window(&window);
	
	return 0;
}