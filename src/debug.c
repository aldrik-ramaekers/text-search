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

#define TARGET_FRAMERATE 1000/30.0

static bool is_window_active = false;
static platform_window global_debug_window;

void debug_hide_window()
{
	if (platform_window_is_valid(&global_debug_window))
	{
		platform_destroy_window(&global_debug_window);
	}
}

void *debug_thread(void *args)
{
	return 0;
	global_debug_window = platform_open_window("Debug view", 800, 600, 800, 600);
	
	keyboard_input keyboard = keyboard_input_create();
	mouse_input mouse = mouse_input_create();
	
	camera camera;
	camera.x = 0;
	camera.y = 0;
	camera.rotation = 0;
	
	font* font_small = assets_load_font("data/fonts/mono.ttf", 16);
	
	while(global_debug_window.is_open) {
		u64 last_stamp = platform_get_time(TIME_FULL, TIME_US);
		platform_window_make_current(&global_debug_window);
		platform_handle_events(&global_debug_window, &mouse, &keyboard);
		platform_set_cursor(&global_debug_window, CURSOR_DEFAULT);
		
		glClearColor(255/255.0, 255/255.0, 255/255.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		camera_apply_transformations(&global_debug_window, &camera);
		
		u64 current_stamp = platform_get_time(TIME_FULL, TIME_US);
		u64 diff = current_stamp - last_stamp;
		float diff_ms = diff / 1000.0f;
		last_stamp = current_stamp;
		
		if (diff_ms < TARGET_FRAMERATE)
		{
			double time_to_wait = (TARGET_FRAMERATE) - diff_ms;
			thread_sleep(time_to_wait*1000);
		}
        
		platform_window_swap_buffers(&global_debug_window);
	}
	
	assets_destroy_font(font_small);
	
	keyboard_input_destroy(&keyboard);
	platform_destroy_window(&global_debug_window);
	
	return 0;
}

void debug_init()
{
	thread t = thread_start(debug_thread, 0);
	thread_detach(&t);
}