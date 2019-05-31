#include "config.h"
#include "project_base.h"

array files;
u64 find_duration_us = 0;
char *find_status_text;
bool found_matches = false;

static void* find_text_in_files_t(void *arg)
{
	found_matches = false;
	
	u64 start_f = platform_get_time(TIME_FULL, TIME_US);
	
	char *text_to_find = arg;
	for (s32 i = 0; i < files.length; i++)
	{
		char **full_path = array_at(&files, i);
		file_content content = platform_read_file_content(*full_path, "r");
		platform_destroy_file_content(&content);
	}
	u64 end_f = platform_get_time(TIME_FULL, TIME_US);
	
	find_duration_us = end_f - start_f;
	sprintf(find_status_text, "found %d matched files in %luus", files.length, find_duration_us);
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

int main(int argc, char **argv)
{
	platform_window window = platform_open_window("test window", 800, 800);
	
	assets_create();
	audio_system_create();
	
#ifdef MODE_DEVELOPER
	info_menu_create();
#endif
	
	image *search_img = assets_load_image("data/search.png");
	font *font_small = assets_load_font("data/mono.ttf", 16);
	
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
	button_state button_select_directory = ui_create_button();
	button_state button_find_text = ui_create_button();
	
	find_status_text = malloc(200);
	
	files = array_create(sizeof(char*));
	array_reserve(&files, 5000);
	
#ifdef MODE_DEVELOPER
	strcpy(textbox_path.buffer, "/home/aldrik/Projects/project-base/src");
#endif
	
	while(window.is_open) {
		platform_handle_events(&window, &mouse, &keyboard);
		
		glClearColor(220/255.0, 220/255.0, 220/255.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		camera_apply_transformations(&window, &camera);
		
		// begin ui
		ui_begin();
		{
			global_ui_context.style.background = rgb(170, 170, 170);
			global_ui_context.style.foreground = rgb(10, 10, 10);
			global_ui_context.style.textbox_background = rgb(200,200,200);
			global_ui_context.style.textbox_foreground = rgb(10,10,10);
			global_ui_context.style.textbox_active_border = rgb(0,0,200);
			
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
			
			global_ui_context.style.background = rgb(220, 220, 220);
			
			ui_push_separator();
			
			ui_block_begin(LAYOUT_HORIZONTAL);
			{
				ui_push_textbox(&textbox_path, "Search directory");
				if (ui_push_button_image(&button_select_directory, "", search_img))
				{
					platform_open_file_dialog(OPEN_DIRECTORY, textbox_path.buffer);
				}
				ui_push_checkbox(&checkbox_recursive, "Recursive");
			}
			ui_block_end();
			
			global_ui_context.layout.offset_y -= 5;
			
			ui_block_begin(LAYOUT_HORIZONTAL);
			{
				ui_push_textbox(&textbox_search_text, "Text to find..");
				if (ui_push_button_image(&button_find_text, "", search_img))
				{
					files.length = 0;
					u64 start_f = platform_get_time(TIME_FULL, TIME_US);
					platform_list_files(&files, textbox_path.buffer, checkbox_recursive.state);
					printf("file find time: %luus [%d files]\n", platform_get_time(TIME_FULL, TIME_US) - start_f, files.length);
					
					find_text_in_files(textbox_search_text.buffer);
				}
			}
			ui_block_end();
			
			ui_push_separator();
		}
		ui_end();
		// end ui
		
		// draw found files
		{
			s32 y = global_ui_context.layout.offset_y;
			
			if (found_matches)
			{
				s32 text_size = calculate_text_width(font_small, find_status_text);
				render_text(font_small, window.width - text_size - 8, 42, find_status_text, rgb(10,10,10));
			}
			
			// TODO(Aldrik): draw result
		}
		
		// TODO(Aldrik): we should check how long we have left and maybe skip this if load is heavy
		assets_do_post_process();
		
		platform_window_swap_buffers(&window);
		
		mouse.left_state &= ~MOUSE_CLICK;
		mouse.left_state &= ~MOUSE_RELEASE;
	}
	
	thread_stop(&asset_queue_worker1);
	
#ifdef MODE_DEVELOPER
	info_menu_destroy();
#endif
	
	ui_destroy();
	
	array_destroy(&files);
	free(find_status_text);
	
	assets_destroy_image(search_img);
	assets_destroy_font(font_small);
	assets_destroy();
	audio_system_destroy();
	
	keyboard_input_destroy(&keyboard);
	platform_close_window(&window);
	
	return 0;
}