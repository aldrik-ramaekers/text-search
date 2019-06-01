#include "config.h"
#include "project_base.h"

typedef struct t_text_match
{
	found_file file;
	u32 match_count;
} text_match;

array files;
u64 find_duration_us = 0;
char *find_status_text;
bool show_error_message = false;
array errors;
bool found_matches = false;
s32 search_result_source_dir_len;
s32 files_searched = 0;

image *search_img;
image *error_img;
image *directory_img;
font *font_small;
font *font_mini;

// TODO(Aldrik): refactor globals into structs
// TODO(Aldrik): make constant ui max input len and check everywhere, also in platform layer.

static void* find_text_in_files_t(void *arg)
{
	found_matches = false;
	
	u64 start_f = platform_get_time(TIME_FULL, TIME_US);
	
	char *text_to_find = arg;
	for (s32 i = 0; i < files.length; i++)
	{
		char **full_path = array_at(&files, i);
		files_searched++;
		//file_content content = platform_read_file_content(*full_path, "r");
		//platform_destroy_file_content(&content);
	}
	u64 end_f = platform_get_time(TIME_FULL, TIME_US);
	
	find_duration_us = end_f - start_f;
	sprintf(find_status_text, "%d out of %d files matched in %.2fms", files.length, files.length, find_duration_us/1000.0);
	printf("%s\n", find_status_text);
	
	found_matches = true;
	
	return 0;
}

static void find_text_in_files(char *text_to_find)
{
	//find_text_in_files_t(text_to_find);
	thread thr = thread_start(find_text_in_files_t, text_to_find);
	thread_detach(&thr);
}

static void render_status_bar(platform_window *window, font *font_small)
{
	s32 h = 30;
	s32 y = window->height - h;
	
	s32 text_size = calculate_text_width(font_small, find_status_text);
	render_rectangle(-1, y, window->width-2, h, rgb(225,225,225));
	render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
	render_text(font_small, window->width - text_size - 8, y + (h/2)-(font_small->size/2) + 1, find_status_text, global_ui_context.style.foreground);
}

static void render_result(platform_window *window, font *font_small)
{
	s32 y = global_ui_context.layout.offset_y;
	s32 h = 24;
	
	s32 render_y = y - 8;
	s32 render_h = window->height - 30 - render_y; // TODO(Aldrik): make constants..
	render_set_scissor(window, 0, render_y, window->width, render_h);
	
	if (files.length)
	{
		render_rectangle(0, y-8, (files_searched/files.length)*window->width, 20, rgb(0,200,0));
		y += 11;
		
		for (s32 i = 0; i < files.length; i++)
		{
			text_match *match = array_at(&files, i);
			render_rectangle_outline(-1, y, window->width+2, h, 1, global_ui_context.style.border);
			render_text(font_small, 10, y + (h/2)-(font_small->size/2) + 1, match->file.path, global_ui_context.style.foreground);
			
			y += h-1;
		}
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
	if (show_error_message)
	{
		s32 h = 30;
		s32 yy = global_ui_context.layout.offset_y;
		
		s32 img_size = 16;
		
		for (s32 e = 0; e < errors.length; e++)
		{
			char *message = array_at(&errors, e);
			
			render_image(error_img, 6, yy + (h/2) - (img_size/2), img_size, img_size);
			render_text(font_small, 12 + img_size, yy + (h/2)-(font_small->size/2) + 1, message, rgb(224, 79, 95));
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

static void prepare_search_directory_path(char *path, s32 len)
{
	// TODO(Aldrik): check max length
	if (path[len-1] != '/')
	{
		path[len] = '/';
		path[len+1] = 0;
	}
}

static void clear_errors()
{
	errors.length = 0;
	show_error_message = false;
}

static void set_error(char *message)
{
	show_error_message = true;
	found_matches = false;
	
	array_push(&errors, message);
}

static void reset_status_text()
{
	strcpy(find_status_text, "No search completed.");
}

int main(int argc, char **argv)
{
	platform_window window = platform_open_window("Text-search", 800, 600);
	
	assets_create();
	audio_system_create();
	
#ifdef MODE_DEVELOPER
	info_menu_create();
#endif
	
	search_img = assets_load_image("data/search.png");
	directory_img = assets_load_image("data/folder.png");
	error_img = assets_load_image("data/error.png");
	font_small = assets_load_font("data/mono.ttf", 16);
	font_mini = assets_load_font("data/mono.ttf", 12);
	
	keyboard_input keyboard = keyboard_input_create();
	mouse_input mouse = mouse_input_create();
	
	camera camera;
	camera.x = 0;
	camera.y = 0;
	camera.rotation = 0;
	
	ui_create(&window, &keyboard, &mouse, &camera, font_small);
	
	thread asset_queue_worker1 = thread_start(assets_queue_worker, NULL);
	thread_detach(&asset_queue_worker1);
	
	checkbox_state checkbox_recursive = ui_create_checkbox(false);
	textbox_state textbox_search_text = ui_create_textbox(4000);
	textbox_state textbox_path = ui_create_textbox(4000);
	textbox_state textbox_file_filter = ui_create_textbox(4000);
	button_state button_select_directory = ui_create_button();
	button_state button_find_text = ui_create_button();
	
	find_status_text = malloc(200);
	
	errors = array_create(70);
	array_reserve(&errors, 5);
	
	reset_status_text();
	
	files = array_create(sizeof(text_match));
	array_reserve(&files, 5000);
	
#ifdef MODE_DEVELOPER
	strcpy(textbox_path.buffer, "/home/aldrik/Projects/text-search/src");
	strcpy(textbox_file_filter.buffer, "*.c");
	strcpy(textbox_search_text.buffer, "hello");
#endif
	
	while(window.is_open) {
		platform_handle_events(&window, &mouse, &keyboard);
		
		glClearColor(255/255.0, 255/255.0, 255/255.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		camera_apply_transformations(&window, &camera);
		
		// begin ui
		ui_begin();
		{
			global_ui_context.style.background_hover = rgb(150, 190, 255);
			global_ui_context.style.background = rgb(225,225,225);
			global_ui_context.style.border = rgb(180,180,180);
			global_ui_context.style.foreground = rgb(10, 10, 10);
			global_ui_context.style.textbox_background = rgb(240,240,240);
			global_ui_context.style.textbox_foreground = rgb(10,10,10);
			global_ui_context.style.textbox_active_border = rgb(0,0,200);
			global_ui_context.style.button_background = rgb(225,225,225);
			
			ui_begin_menu_bar();
			{
				if (ui_push_menu("File"))
				{
					if (ui_push_menu_item("Open", "Ctrl + O")) { }
					if (ui_push_menu_item("Find", "Ctrl + F")) { }
					ui_push_menu_item_separator();
					if (ui_push_menu_item("Close", "Ctrl + C")) { }
				}
				if (ui_push_menu("Options"))
				{
					if (ui_push_menu_item("Edit", "Ctrl + E")) { }
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
						search_result_source_dir_len = strlen(textbox_path.buffer);
						prepare_search_directory_path(textbox_path.buffer, search_result_source_dir_len);
						
						files.length = 0;
						u64 start_f = platform_get_time(TIME_FULL, TIME_US);
						platform_list_files(&files, textbox_path.buffer, textbox_file_filter.buffer, checkbox_recursive.state);
						printf("file find time: %luus [%d files]\n", platform_get_time(TIME_FULL, TIME_US) - start_f, files.length);
						
						find_text_in_files(textbox_search_text.buffer);
					}
				}
				ui_push_checkbox(&checkbox_recursive, "Folders");
			}
			ui_block_end();
			
			ui_push_separator();
		}
		ui_end();
		// end ui
		
		// draw info or results
		{
			render_status_bar(&window, font_small);
			
			if (!found_matches)
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
	
	ui_destroy();
	
	array_destroy(&files);
	array_destroy(&errors);
	free(find_status_text);
	
	assets_destroy_image(search_img);
	assets_destroy_image(directory_img);
	assets_destroy_image(error_img);
	
	assets_destroy_font(font_small);
	assets_destroy();
	audio_system_destroy();
	
	keyboard_input_destroy(&keyboard);
	platform_close_window(&window);
	
	return 0;
}