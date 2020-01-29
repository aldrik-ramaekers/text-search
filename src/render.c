/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
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
		
		glDisable(GL_TEXTURE_2D);
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
		
		glDisable(GL_TEXTURE_2D);
	}
}

s32 render_text(font *font, s32 x, s32 y, char *text, color tint)
{
	if (!font->loaded)
		return 0;
	
	glEnable(GL_TEXTURE_2D);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	
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
		
		glyph g = font->glyphs[ch];
		
		glBindTexture(GL_TEXTURE_2D, g.textureID);
		glBegin(GL_QUADS);
		
		s32 width = g.width;
		
		s32 y_ = y + font->px_h + g.yoff;
		
		glTexCoord2i(0, 0); glVertex3i(x_,y_, render_depth);
		glTexCoord2i(0, 1); glVertex3i(x_,y_+g.height, render_depth);
		glTexCoord2i(1, 1); glVertex3i(x_+g.width,y_+g.height, render_depth);
		glTexCoord2i(1, 0); glVertex3i(x_+g.width,y_, render_depth);
		
		glEnd();
		
		/* add kerning */
		int kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		if (kern != 0) x_ += kern * font->scale;
		x_ += g.width+g.xoff;
	}
	
	glDisable(GL_TEXTURE_2D);
	
	return x_ - x;
}

s32 render_text_cutoff(font *font, s32 x, s32 y, char *text, color tint, u16 cutoff_width)
{
	if (!font->loaded)
		return 0;
	
	glEnable(GL_TEXTURE_2D);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	
	s32 x_ = x;
	s32 y_ = y;
	bool is_new_line = false;
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
		
		
		glyph g = font->glyphs[ch];
		
		glBindTexture(GL_TEXTURE_2D, g.textureID);
		glBegin(GL_QUADS);
		
		s32 width = g.width;
		
		s32 y__ = y_ + font->px_h + g.yoff;
		
		glTexCoord2i(0, 0); glVertex3i(x_,y__, render_depth);
		glTexCoord2i(0, 1); glVertex3i(x_,y__+g.height, render_depth);
		glTexCoord2i(1, 1); glVertex3i(x_+g.width,y__+g.height, render_depth);
		glTexCoord2i(1, 0); glVertex3i(x_+g.width,y__, render_depth);
		
		glEnd();
		
		int kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		if (kern != 0) x_ += kern * font->scale;
		
		x_ += g.width+g.xoff;
		if (x_ > x+cutoff_width)
		{
			x_ = x;
			y_ += font->size;
			is_new_line = true;
		}
	}
	
	glDisable(GL_TEXTURE_2D);
	
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
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		
		glyph g = font->glyphs[ch];
		
		s32 width = g.width;
		s32 width_next = font->glyphs[ch_next].width;
		
		int kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		if (kern != 0) x += kern * font->scale;
		
		x += g.width+g.xoff;
		
		if (x - (width_next/5) > click_x)
		{
			return index;
		}
		
		++index;
	}
	
	return index;
}

s32 calculate_text_width_from_upto(font *font, char *text, s32 from, s32 index)
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
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		glyph g = font->glyphs[ch];
		s32 width = g.width;
		
		if (i >= from)
		{
			int kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
			if (kern != 0) x += kern * font->scale;
			
			x += g.width+g.xoff;
		}
		
		i++;
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
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		glyph g = font->glyphs[ch];
		s32 width = g.width;
		
		int kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		if (kern != 0) x += kern * font->scale;
		
		x += g.width+g.xoff;
		
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
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		
		glyph g = font->glyphs[ch];
		s32 width = g.width;
		
		int kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		if (kern != 0) x += kern * font->scale;
		
		x += g.width+g.xoff;
	}
	
	return x;
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