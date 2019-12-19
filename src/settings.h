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

#ifndef INCLUDE_SETTINGS
#define INCLUDE_SETTINGS

typedef struct t_settings_page
{
	platform_window window;
	keyboard_input keyboard;
	mouse_input mouse;
    camera camera;
	bool active;
	
	font *font_small;
	image *logo_img;
	
	button_state btn_close;
	button_state btn_save;
	dropdown_state dropdown_language;
	textbox_state textbox_max_file_size;
	textbox_state textbox_max_thread_count;
	checkbox_state checkbox_parallelize_search;
	char *current_locale_id;
	
	s32 selected_tab_index;
	s32 max_thread_count;
	s32 max_file_size;
	//bool enable_parallelization;
} settings_page;

settings_page global_settings_page;

void settings_page_create();
void settings_page_hide_without_save();
void settings_page_update_render();
void settings_page_show();
void settings_page_hide();
void settings_page_destroy();

#endif