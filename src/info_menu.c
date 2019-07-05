/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This file is part of Text-search.
*
    *  Text-search is free software: you can redistribute it and/or modify
    *  it under the terms of the GNU General Public License as published by
    *  the Free Software Foundation, either version 3 of the License, or
    *  (at your option) any later version.
	
    *  Text-search is distributed in the hope that it will be useful,
    *  but WITHOUT ANY WARRANTY; without even the implied warranty of
    *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    *  GNU General Public License for more details.
	
    *  You should have received a copy of the GNU General Public License
    *  along with Text-search.  If not, see <https://www.gnu.org/licenses/>.
*/

void info_menu_update_render(platform_window *window, camera *camera, keyboard_input *keyboard, mouse_input *mouse)
{
	global_info_menu.button_count = 0;
	
	profiler_update_render(window, window->width-WATCH_WINDOW_WIDTH-20, 0, 
						   WATCH_WINDOW_WIDTH, window->height, camera, global_info_menu.state == INFO_MENU_PROFILER, mouse);
	
	switch(global_info_menu.state)
	{
		case INFO_MENU_SYSTEM:
		system_watch_update_render(
			window, camera, window->width-WATCH_WINDOW_WIDTH-20, 0, WATCH_WINDOW_WIDTH, window->height); break;
		case INFO_MENU_ASSET_PIPELINE:
		asset_pipeline_watch_update_render(window, camera, mouse, window->width-WATCH_WINDOW_WIDTH-20, 0, WATCH_WINDOW_WIDTH, window->height);
		break;
		case INFO_MENU_CONSOLE:
		console_update_render(window, camera, mouse, window->width-WATCH_WINDOW_WIDTH-20, 0, WATCH_WINDOW_WIDTH, window->height);
		break;
	}
	
	if (global_info_menu.state && mouse->x < window->width-WATCH_WINDOW_WIDTH-20)
	{
		global_info_menu.state = 0;
	}
	
	if (info_menu_push_button(window, camera, mouse, "profiler"))
	{
		global_info_menu.state = INFO_MENU_PROFILER;
	}
	if (info_menu_push_button(window, camera, mouse, "assets"))
	{
		global_info_menu.state = INFO_MENU_ASSET_PIPELINE;
	}
	if (info_menu_push_button(window, camera, mouse, "console"))
	{
		global_info_menu.state = INFO_MENU_CONSOLE;
	}
	if (info_menu_push_button(window, camera, mouse, "system"))
	{
		global_info_menu.state = INFO_MENU_SYSTEM;
	}
}

u8 info_menu_push_button(platform_window *window, camera *camera, mouse_input *mouse, char *title)
{
	int w = 20;
	int h = 110;
	int x = window->width-w;
	int y = global_info_menu.button_count*h;
	
	bool hovered = (mouse->x >= x && mouse->x <= x + w && mouse->y >= y && mouse->y < y + h);
	
	if (!hovered)
		render_rectangle(x+camera->x, y+camera->y, w, h, WATCH_WINDOW_HOVER_BACKGROUND_COLOR);
	else
		render_rectangle(x+camera->x, y+camera->y, w, h, WATCH_WINDOW_BACKGROUND_COLOR);
	
	render_rectangle_outline(x+camera->x, y+camera->y, w, h, 1, WATCH_WINDOW_BORDER_COLOR);
	render_text_vertical(global_info_menu.font_small, x+5+camera->x, y+5+camera->y, title, TOOLTIP_FOREGROUND_COLOR);
	
	global_info_menu.button_count++;
	
	return hovered;
}

inline void info_menu_create()
{
	profiler_create();
	console_create();
	
	global_info_menu.font_small = assets_load_font("data/mono.ttf", 16);
	global_info_menu.font_medium = assets_load_font("data/mono.ttf", 20);
}

inline void info_menu_destroy()
{
	profiler_destroy();
	console_destroy();
	
	assets_destroy_font(global_info_menu.font_small);
	assets_destroy_font(global_info_menu.font_medium);
}