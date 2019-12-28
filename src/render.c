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

inline void render_clear()
{
	glClearColor(255/255.0, 255/255.0, 255/255.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void render_set_rotation(float32 rotation, float32 x, float32 y, s32 depth)
{
	glRotatef(rotation, x, y, depth);
}

inline void set_render_depth(s32 depth)
{
	render_depth = depth;
}

void render_image(image *image, s32 x, s32 y, s32 width, s32 height)
{
	assert(image);
	if (image->loaded)
	{
		glBindTexture(GL_TEXTURE_2D, image->textureID);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(1., 1., 1., 1.);
		glTexCoord2i(0, 0); glVertex3i(x, y, render_depth);
		glTexCoord2i(0, 1); glVertex3i(x, y+height, render_depth);
		glTexCoord2i(1, 1); glVertex3i(x+width, y+height, render_depth);
		glTexCoord2i(1, 0); glVertex3i(x+width, y, render_depth);
		glEnd();
		
		glFlush();
		
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void render_image_tint(image *image, s32 x, s32 y, s32 width, s32 height, color tint)
{
	assert(image);
	if (image->loaded)
	{
		glBindTexture(GL_TEXTURE_2D, image->textureID);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
		glTexCoord2i(0, 0); glVertex3i(x, y, render_depth);
		glTexCoord2i(0, 1); glVertex3i(x, y+height, render_depth);
		glTexCoord2i(1, 1); glVertex3i(x+width, y+height, render_depth);
		glTexCoord2i(1, 0); glVertex3i(x+width, y, render_depth);
		glEnd();
		
		glFlush();
		
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void render_font_palette(font *font, s32 x, s32 y, s32 w, s32 h, color tint)
{
	if (!font->loaded)
		return;
	
	for (s32 i = 0; i < TOTAL_GLYPH_BITMAPS; i++)
	{
		glBindTexture(GL_TEXTURE_2D, font->textureIDs[i]);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
		
		glTexCoord2i(0, 0); glVertex3i(x, y, render_depth);
		glTexCoord2i(0, 1); glVertex3i(x, y+h, render_depth);
		glTexCoord2i(1, 1); glVertex3i(x+w, y+h, render_depth);
		glTexCoord2i(1, 0); glVertex3i(x+w, y, render_depth);
		
		x += w;
		
		glEnd();
		glFlush();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

static s32 add_char_width(char ch, s32 width, font *font)
{
	if (ch != '.' && ch != ',' && ch != ':' && ch != '(' && ch != ')' && ch != '!' && 
		ch != ';' && ch != '`')
		return width/2;
	else
		return font->size/4;
}

s32 render_text(font *font, s32 x, s32 y, char *text, color tint)
{
	if (!font->loaded)
		return 0;
	
	s32 x_ = x;
	utf8_int32_t ch;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch == 9) ch = 32;
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		s32 bitmap_index = ch / GLYPHS_PER_BITMAP;
		
		glBindTexture(GL_TEXTURE_2D, font->textureIDs[bitmap_index]);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
		
		s32 offsetx = (font->size*2)*(ch%GLYPHS_PER_BITMAP);
		
		float ipw = 1.0f / font->palette_width, iph = 1.0f / font->palette_height;
		
		float sx0, sy0, sx1, sy1;
		sx0 = ipw*offsetx;
		sy0 = 0;
		sx1 = ipw*(offsetx+font->size*2);
		sy1 = iph*font->size*2;
		
		s32 width = font->glyph_widths[ch];
		
		glTexCoord2f(sx0,sy0); glVertex3i(x_,y, render_depth);
		glTexCoord2f(sx0,sy1); glVertex3i(x_,y+font->size, render_depth);
		glTexCoord2f(sx1,sy1); glVertex3i(x_+font->size,y+font->size, render_depth);
		glTexCoord2f(sx1,sy0); glVertex3i(x_+font->size,y, render_depth);
		
		glEnd();
		glFlush();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		/* add kerning */
		int kern = 0;
		kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		{
			if (kern != 0) printf("%d\n", kern);
		}
		
		x_ += add_char_width(ch,width,font);
	}
	
	return x_ - x;
}

s32 render_text_cutoff(font *font, s32 x, s32 y, char *text, color tint, u16 cutoff_width)
{
	if (!font->loaded)
		return 0;
	
	s32 x_ = x;
	s32 y_ = y;
	bool is_new_line = false;
	utf8_int32_t ch;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch == 9) ch = 32;
		char ch_next = *(text+1);
		s32 offsetx = (font->size*2)*(ch%GLYPHS_PER_BITMAP);
		
		if (ch == '\n')
		{
			x_ = x;
			y_ += font->size;
			is_new_line = true;
			continue;
		}
		
		if (is_new_line && ch == ' ')
		{
			is_new_line = false;
			continue;
		}
		else if (is_new_line && ch != ' ')
		{
			is_new_line = false;
		}
		
		s32 bitmap_index = ch / GLYPHS_PER_BITMAP;
		
		glBindTexture(GL_TEXTURE_2D, font->textureIDs[bitmap_index]);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
		
		float ipw = 1.0f / font->palette_width, iph = 1.0f / font->palette_height;
		
		float sx0, sy0, sx1, sy1;
		sx0 = ipw*offsetx;
		sy0 = 0;
		sx1 = ipw*(offsetx+font->size*2);
		sy1 = iph*font->size*2;
		
		s32 width = font->glyph_widths[ch];
		
		glTexCoord2f(sx0,sy0); glVertex3i(x_,y_, render_depth);
		glTexCoord2f(sx0,sy1); glVertex3i(x_,y_+font->size, render_depth);
		glTexCoord2f(sx1,sy1); glVertex3i(x_+font->size,y_+font->size, render_depth);
		glTexCoord2f(sx1,sy0); glVertex3i(x_+font->size,y_, render_depth);
		
		glEnd();
		glFlush();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		/* add kerning */
        //int kern = 0;
		
		//if (ch_next != 0)
		//{
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		
		//if (kern != 0) printf("%d\n", kern);
		//}
		
		x_ += add_char_width(ch,width,font);
		
		if (x_ > x+cutoff_width)
		{
			x_ = x;
			y_ += font->size;
			is_new_line = true;
		}
	}
	
	return (y_ - y) + font->size;
	
}

s32 calculate_cursor_position(font *font, char *text, s32 click_x)
{
	if (!font->loaded)
		return 0;
	
	s32 x = 0;
	s32 index = 0;
	utf8_int32_t ch;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch == 9) ch = 32;
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		s32 width = font->glyph_widths[ch];
		
		x += add_char_width(ch,width,font);
		
		if (x > click_x)
		{
			return index-1;
		}
		
		++index;
	}
	
	return x;
}

s32 calculate_text_width_upto(font *font, char *text, s32 index)
{
	if (!font->loaded)
		return 0;
	
	s32 x = 0;
	utf8_int32_t ch;
	s32 i = 0;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (index == i) return x;
		
		if (ch == 9) ch = 32;
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		s32 width = font->glyph_widths[ch];
		
		x += add_char_width(ch,width,font);
		
		i++;
	}
	
	return x;
}

s32 calculate_text_width(font *font, char *text)
{
	if (!font->loaded)
		return 0;
	
	s32 x = 0;
	utf8_int32_t ch;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch == 9) ch = 32;
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		s32 width = font->glyph_widths[ch];
		
		x += add_char_width(ch,width,font);
	}
	
	return x;
}

s32 calculate_text_height(font *font, s32 cutoff_width, char *text)
{
	if (!font->loaded)
		return 0;
	
	s32 x_ = 0;
	s32 y = 0;
	utf8_int32_t ch;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch == 9) ch = 32;
		char ch_next = *(text+1);
		s32 width = font->glyph_widths[ch];
		
		x_ += width / 2;
		
		if (x_ > cutoff_width)
		{
			x_ = 0;
			y += font->size;
		}
	}
	
	return y + font->size;
}

void render_triangle(s32 x, s32 y, s32 w, s32 h, color tint)
{
	glBegin(GL_TRIANGLES);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	glVertex3i(x+(w/2), y+h, render_depth);
	glVertex3i(x, y, render_depth);
	glVertex3i(x+w, y, render_depth);
	glEnd();
}

void render_rectangle(s32 x, s32 y, s32 width, s32 height, color tint)
{
	glBegin(GL_QUADS);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	glVertex3i(x, y, render_depth);
	glVertex3i(x, y+height, render_depth);
	glVertex3i(x+width, y+height, render_depth);
	glVertex3i(x+width, y, render_depth);
	glEnd();
}

void render_rectangle_tint(s32 x, s32 y, s32 width, s32 height, color tint[4])
{
	glBegin(GL_QUADS);
	glColor4f(tint[0].r/255.0f, tint[0].g/255.0f, tint[0].b/255.0f, tint[0].a/255.0f);
	glVertex3i(x, y, render_depth);
	glColor4f(tint[1].r/255.0f, tint[1].g/255.0f, tint[1].b/255.0f, tint[1].a/255.0f);
	glVertex3i(x, y+height, render_depth);
	glColor4f(tint[2].r/255.0f, tint[2].g/255.0f, tint[2].b/255.0f, tint[2].a/255.0f); 
	glVertex3i(x+width, y+height, render_depth);
	glColor4f(tint[3].r/255.0f, tint[3].g/255.0f, tint[3].b/255.0f, tint[3].a/255.0f);
	glVertex3i(x+width, y, render_depth);
	glEnd();
}

void render_rectangle_outline(s32 x, s32 y, s32 width, s32 height, u16 outline_w, color tint)
{
	// left
	render_rectangle(x, y, outline_w, height, tint);
	// right
	render_rectangle(x+width-outline_w, y, outline_w, height, tint);
	// top
	render_rectangle(x+outline_w, y, width-(outline_w*2), outline_w, tint);
	// bottom
	render_rectangle(x+outline_w, y+height-outline_w, width-(outline_w*2), outline_w, tint);
}

void render_set_scissor(platform_window *window, s32 x, s32 y, s32 w, s32 h)
{
	glEnable(GL_SCISSOR_TEST);
	glScissor(x-1, window->height-h-y-1, w+1, h+1);
}

vec4 render_get_scissor()
{
	vec4 vec;
	glGetIntegerv(GL_SCISSOR_BOX, (GLint*)(&vec));
	vec.x += 1;
	vec.y += 1;
	vec.w -= 1;
	vec.h -= 1;
	return vec;
}

void render_reset_scissor()
{
	glDisable(GL_SCISSOR_TEST);
}