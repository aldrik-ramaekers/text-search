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

typedef struct t_debug_view
{
	bool active;
	platform_window window;
	keyboard_input keyboard;
	mouse_input mouse;
	camera camera;
	font *font_small;
} debug_view;

debug_view global_debug_view;

void debug_window_toggle()
{
	if (platform_window_is_valid(&global_debug_view.window))
	{
		global_debug_view.active = false;
		platform_destroy_window(&global_debug_view.window);
	}
	else
	{
		global_debug_view.active = true;
		global_debug_view.window = platform_open_window("Debug view", 800, 600, 800, 600);
		platform_window_set_position(&global_debug_view.window, 100, 100);
	}
}

void debug_init()
{
	global_debug_view.keyboard = keyboard_input_create();
	global_debug_view.mouse = mouse_input_create();
	
	global_debug_view.camera.x = 0;
	global_debug_view.camera.y = 0;
	global_debug_view.camera.rotation = 0;
	
	global_debug_view.font_small = assets_load_font("data/fonts/mono.ttf", 16);
}

void debug_update_render()
{
	if (global_debug_view.active) {
		platform_window_make_current(&global_debug_view.window);
		platform_handle_events(&global_debug_view.window, &global_debug_view.mouse, &global_debug_view.keyboard);
		platform_set_cursor(&global_debug_view.window, CURSOR_DEFAULT);
		
		render_clear();
		
		camera_apply_transformations(&global_debug_view.window, &global_debug_view.camera);
		
		///
		
		render_rectangle(10, 10, 50, 50, rgb(200,0,0));
		
		///
		
		platform_window_swap_buffers(&global_debug_view.window);
		
		if (!global_debug_view.window.is_open)
		{
			debug_window_toggle();
		}
	}
}

void debug_destroy()
{
	assets_destroy_font(global_debug_view.font_small);
	
	keyboard_input_destroy(&global_debug_view.keyboard);
	
	if (global_debug_view.active)
	{
		platform_destroy_window(&global_debug_view.window);
	}
}