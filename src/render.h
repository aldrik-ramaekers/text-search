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


#ifndef INCLUDE_RENDER
#define INCLUDE_RENDER

typedef struct t_color {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
} color;

typedef struct t_vec4
{
	s32 x;
	s32 y;
	s32 w;
	s32 h;
} vec4;

s32 render_depth = 1;
void set_render_depth(s32 depth);

#define rgb(r_,g_,b_) (color){ r_, g_, b_, 255 }
#define rgba(r_,g_,b_,a_) (color){r_,g_,b_,a_}

// images
void render_image(image *image, s32 x, s32 y, s32 width, s32 height);
void render_image_tint(image *image, s32 x, s32 y, s32 width, s32 height, color tint);

// text
void render_font_palette(font *font, s32 x, s32 y, s32 w, s32 h, color tint);
s32 render_text(font *font, s32 x, s32 y, char *text, color tint);
s32 render_text_cutoff(font *font, s32 x, s32 y, char *text, color tint, u16 cutoff_width);
s32 render_text_vertical(font *font, s32 x, s32 y, char *text, color tint);

s32 calculate_cursor_position(font *font, char *text, s32 click_x);
s32 calculate_text_width(font *font, char *text);
s32 calculate_text_height(font *font, s32 cutoff_width, char *text);

// primitives
void render_rectangle(s32 x, s32 y, s32 width, s32 height, color tint);
void render_rectangle_tint(s32 x, s32 y, s32 width, s32 height, color tint[4]);
void render_rectangle_outline(s32 x, s32 y, s32 width, s32 height, u16 outline_w, color tint);
void render_triangle(s32 x, s32 y, s32 w, s32 h, color tint);

// utils
void render_set_scissor(platform_window *window, s32 x, s32 y, s32 w, s32 h);
vec4 render_get_scissor();
void render_reset_scissor();

void render_set_rotation(float32 rotation, float32 x, float32 y, s32 depth);

#endif