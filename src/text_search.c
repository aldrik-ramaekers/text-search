#include "config.h"
#include "project_base.h"

int main(int argc, char **argv)
{
	platform_window window = platform_open_window("test window", 800, 800);
	
	audio_system_create();
	assets_create();
	
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
	thread asset_queue_worker2 = thread_start(assets_queue_worker, NULL);
	thread asset_queue_worker3 = thread_start(assets_queue_worker, NULL);
	thread asset_queue_worker4 = thread_start(assets_queue_worker, NULL);
	
	thread_detach(&asset_queue_worker1);
	thread_detach(&asset_queue_worker2);
	thread_detach(&asset_queue_worker3);
	thread_detach(&asset_queue_worker4);
	
	checkbox_state checkbox_recursive = ui_create_checkbox(false);
	textbox_state textbox_search_text = ui_create_textbox(4000);
	textbox_state textbox_path = ui_create_textbox(4000);
	button_state button_select_directory = ui_create_button();
	button_state button_find_text = ui_create_button();
	
	u64 stamp = platform_get_time(TIME_FULL, TIME_NS);
	while(window.is_open) {
#ifdef MODE_DEVELOPER
		profiler_begin(profiler_start);
#endif
		
		platform_handle_events(&window, &mouse, &keyboard);
		
		glClearColor(220/255.0, 220/255.0, 220/255.0, 1.0);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glPushMatrix();
		
		camera_apply_transformations(&window, &camera);
		
		// begin ui
		ui_begin();
		{
			global_ui_context.style.background = rgb(220, 220, 220);
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
					u64 start_f = platform_get_time(TIME_FULL, TIME_MS);
					array files = array_create(sizeof(char*));
					array_reserve(&files, 5000);
					platform_list_files(&files, textbox_path.buffer, true);
					printf("file find time: %lums [%d files]\n", platform_get_time(TIME_FULL, TIME_MS) - start_f, files.length);
					array_destroy(&files);
					
				}
			}
			ui_block_end();
			
			ui_push_separator();
		}
		ui_end();
		// end ui
		
		
#ifdef MODE_DEVELOPER
		info_menu_update_render(&window, &camera, &keyboard, &mouse);
#endif
		
		glPopMatrix();
		
		// TODO(Aldrik): we should check how long we have left and maybe skip this if load is heavy
		assets_do_post_process();
		
		platform_window_swap_buffers(&window);
		
		mouse.left_state &= ~MOUSE_CLICK;
		mouse.left_state &= ~MOUSE_RELEASE;
		
#ifdef MODE_DEVELOPER
		profiler_end(profiler_start);
#endif
	}
	
	thread_stop(&asset_queue_worker1);
	thread_stop(&asset_queue_worker2);
	thread_stop(&asset_queue_worker3);
	thread_stop(&asset_queue_worker4);
	
#ifdef MODE_DEVELOPER
	info_menu_destroy();
#endif
	
	ui_destroy();
	
	assets_destroy_image(search_img);
	assets_destroy_font(font_small);
	assets_destroy();
	audio_system_destroy();
	
	keyboard_input_destroy(&keyboard);
	platform_close_window(&window);
	
	return 0;
}