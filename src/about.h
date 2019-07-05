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


#ifndef INCLUDE_ABOUT
#define INCLUDE_ABOUT

typedef struct t_about_page
{
	platform_window window;
	keyboard_input keyboard;
	mouse_input mouse;
	bool active;
	image *license_img;
	image *logo_img;
	font *font_big;
	font *font_small;
	button_state btn_close;
	button_state btn_website;
} about_page;

about_page global_about_page;

void about_page_create();
void about_page_update_render();
void about_page_show();
void about_page_hide();
void about_page_destroy();

#endif