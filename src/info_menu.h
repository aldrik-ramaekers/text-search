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

#ifndef INCLUDE_INFO_MENU
#define INCLUDE_INFO_MENU

#ifndef WATCH_WINDOW_WIDTH
#define WATCH_WINDOW_WIDTH 355
#endif

#ifndef TOOLTIP_BACKGROUND_COLOR
#define TOOLTIP_BACKGROUND_COLOR rgb(0,0,0)
#endif

#ifndef TOOLTIP_FOREGROUND_COLOR
#define TOOLTIP_FOREGROUND_COLOR rgb(255,255,255)
#endif

#ifndef WATCH_WINDOW_ENTRY_HOVER_BACKGROUMD_COLOR
#define WATCH_WINDOW_ENTRY_HOVER_BACKGROUMD_COLOR rgba(200,0,0,50)
#endif

#ifndef WATCH_WINDOW_BACKGROUND_COLOR
#define WATCH_WINDOW_BACKGROUND_COLOR rgb(103, 202, 81)
#endif

#ifndef WATCH_WINDOW_HOVER_BACKGROUND_COLOR
#define WATCH_WINDOW_HOVER_BACKGROUND_COLOR rgb(93, 192, 71)
#endif

#ifndef WATCH_WINDOW_BORDER_COLOR
#define WATCH_WINDOW_BORDER_COLOR rgb(73, 172, 51)
#endif

#ifndef WATCH_WINDOW_SCROLL_SPEED
#define WATCH_WINDOW_SCROLL_SPEED 30
#endif

typedef enum t_info_menu_state
{
	INFO_MENU_NONE,
	INFO_MENU_PROFILER,
	INFO_MENU_SYSTEM,
	INFO_MENU_ASSET_PIPELINE,
	INFO_MENU_CONSOLE,
} info_menu_state;

typedef struct t_info_menu
{
	u8 state;
	u8 button_count;
	font *font_small;
	font *font_medium;
} info_menu;

info_menu global_info_menu;
profiler global_profiler;
system_info global_system_watch;
console global_console;

void info_menu_create();
void info_menu_update_render(platform_window *window, camera *camera, keyboard_input *keyboard, mouse_input *mouse);
u8 info_menu_push_button(platform_window *window, camera *camera, mouse_input *mouse, char *title);
void info_menu_destroy();

#endif