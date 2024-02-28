/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

static platform_window *drawing_window = 0;

static void _copy_image_pixel(s32 x, s32 y, image *image, render_target rec)
{
	s32 offset = (y * (drawing_window->backbuffer.width) * 5) + x * 5;
	u8 *buffer_entry = drawing_window->backbuffer.buffer+offset;
	
	if (buffer_entry[4] > render_depth) return;
	buffer_entry[4] = render_depth;
	
	s32 _x = x - rec.x + rec.offset_x;
	s32 _y = y - rec.y + rec.offset_y;
	s32 image_offset = (_y * (image->width) * 4) + _x * 4;
	u8 *color = image->data+image_offset;
	
	float32 alpha = color[3] / 255.0f;
	float32 oneminusalpha = 1 - alpha;
	
	u8 b = ((color[0] * alpha) + (oneminusalpha * buffer_entry[0]));
	u8 g = ((color[1] * alpha) + (oneminusalpha * buffer_entry[1]));
	u8 r = ((color[2] * alpha) + (oneminusalpha * buffer_entry[2]));
	u8 a = color[3];
	
	s32 c = (a << 24) | (r << 16) | (g << 8) | (b << 0);
	
	memcpy(buffer_entry, &c, 4);
}

static void _copy_glyph_pixel(s32 x, s32 y, glyph *gl, render_target rec, color tint)
{
	s32 offset = (y * (drawing_window->backbuffer.width) * 5) + x * 5;
	u8 *buffer_entry = drawing_window->backbuffer.buffer+offset;
	
	if (buffer_entry[4] > render_depth) return;
	buffer_entry[4] = render_depth;
	
	s32 _x = x - rec.x + rec.offset_x;
	s32 _y = y - rec.y + rec.offset_y;
	s32 image_offset = (_y * gl->width) + _x;
	u8 *color = gl->bitmap+image_offset;
	
	float32 alpha = color[0] / 255.0f;
	float32 oneminusalpha = 1 - alpha;
	
	u8 r = ((tint.r * alpha) + (oneminusalpha * buffer_entry[2]));
	u8 g = ((tint.g * alpha) + (oneminusalpha * buffer_entry[1]));
	u8 b = ((tint.b * alpha) + (oneminusalpha * buffer_entry[0]));
	u8 a = color[0];
	
	s32 c = (a << 24) | (r << 16) | (g << 8) | (b << 0);
	
	memcpy(buffer_entry, &c, 4);
}

static void _set_pixel(s32 x, s32 y, color tint)
{
	s32 offset = (y * (drawing_window->backbuffer.width) * 5) + x * 5;
	u8 *buffer_entry = drawing_window->backbuffer.buffer+offset;
	
	if (buffer_entry[4] > render_depth) return;
	buffer_entry[4] = render_depth;
	
	float32 alpha = tint.a / 255.0f;
	float32 oneminusalpha = 1 - alpha;
	
	u8 r = ((tint.r * alpha) + (oneminusalpha * buffer_entry[2]));
	u8 g = ((tint.g * alpha) + (oneminusalpha * buffer_entry[1]));
	u8 b = ((tint.b * alpha) + (oneminusalpha * buffer_entry[0]));
	u8 a = tint.a;
	
	s32 c = (a << 24) | (r << 16) | (g << 8) | (b << 0);
	
	memcpy(buffer_entry, &c, 4);
}

// returns topleft and bottomright corners. not width + height
static render_target _get_actual_rect(s32 x, s32 y, s32 width, s32 height)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	s32 start_x = x;
	s32 start_y = y;
	s32 end_x = start_x + width;
	s32 end_y = start_y + height;
	s32 offset_x = 0;
	s32 offset_y = 0;
	
	if (start_x < current_scissor.x) { 
		offset_x = current_scissor.x - start_x;
		start_x = current_scissor.x;
	}
	if (start_y < current_scissor.y) {
		offset_y = current_scissor.y - start_y;
		start_y = current_scissor.y;
	}
	
	if (end_x > current_scissor.x+current_scissor.w) 
		end_x = current_scissor.x+current_scissor.w;
	if (end_y > current_scissor.y+current_scissor.h) 
		end_y = current_scissor.y+current_scissor.h;
	
	if (end_x > drawing_window->backbuffer.width)
		end_x = drawing_window->backbuffer.width;
	if (end_y > drawing_window->backbuffer.height)
		end_y = drawing_window->backbuffer.height;
	
	return (render_target){start_x,start_y,end_x,end_y,offset_x,offset_y};
}

inline void render_clear(platform_window *window)
{
	if (global_use_gpu)
	{
		glClearColor(255/255.0, 255/255.0, 255/255.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	else
	{
		drawing_window = window;
		render_reset_scissor();
		
		render_depth = 1;
		
		u8 pixel[5] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
		s32 pixel_count = window->backbuffer.width*window->backbuffer.height;
		for (s32 i = 0; i < pixel_count; i++)
			memcpy(window->backbuffer.buffer+(i*5), pixel, 5);
	}
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
	
	if (global_use_gpu)
	{
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
	else
	{
		if (image->loaded)
		{
			render_target rec = _get_actual_rect(x, y, image->width, image->height);
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				for (s32 x = rec.x; x < rec.w; x++)
				{
					_copy_image_pixel(x, y, image, rec);
				}
			}
		}
	}
}

void render_image_tint(image *image, s32 x, s32 y, s32 width, s32 height, color tint)
{
	assert(image);
	if (image->loaded)
	{
		if (global_use_gpu)
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
		else
		{
			assert(0 && "not implemented");
		}
	}
}

s32 render_text_ellipsed(font *font, s32 x, s32 y, s32 maxw, char *text, color tint)
{
	if (!font->loaded)
		return 0;
	
	if (global_use_gpu)
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	}
	
	char *ellipse = "...";
	bool in_ellipse = false;
	
	s32 len = utf8len(text);
	
	s32 x_ = x;
	utf8_int32_t ch;
	s32 count = 0;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		count++;
		if (ch == 9) ch = 32;
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		if (ch == '\n') ch = 0xB6;
		
		glyph g = font->glyphs[ch];
		
		s32 y_ = y + font->px_h + g.yoff;
		s32 x_to_render = x_ + (g.lsb);
		
		if (global_use_gpu)
		{
			glBindTexture(GL_TEXTURE_2D, g.textureID);
			glBegin(GL_QUADS);
			
			s32 width = g.width;
			
			glTexCoord2i(0, 0); glVertex3i(x_to_render,y_, render_depth);
			glTexCoord2i(0, 1); glVertex3i(x_to_render,y_+g.height, render_depth);
			glTexCoord2i(1, 1); glVertex3i(x_to_render+g.width,y_+g.height, render_depth);
			glTexCoord2i(1, 0); glVertex3i(x_to_render+g.width,y_, render_depth);
			
			glEnd();
		}
		else
		{
			render_target rec = _get_actual_rect(x_to_render, y_, g.width, g.height);
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				for (s32 x = rec.x; x < rec.w; x++)
				{
					_copy_glyph_pixel(x, y, &g, rec, tint);
				}
			}
		}
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x_ += (g.advance);
		
		if (!in_ellipse && (x_-x) > maxw-(font->glyphs['.'].width*3) && count < len-3)
		{
			in_ellipse = true;
			text = ellipse;
		}
	}
	
	if (global_use_gpu)
		glDisable(GL_TEXTURE_2D);
	
	return maxw;
}

s32 render_text_with_selection(font *font, s32 x, s32 y, char *text, color tint, s32 selection_start, s32 selection_length)
{
	if (!font->loaded)
		return 0;
	
	if (global_use_gpu)
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	}
	
	s32 x_ = x;
	utf8_int32_t ch;
	s32 index = 0;
	s32 selection_start_x = x_;
	s32 selection_end_x = x_;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch == 9) ch = 32;
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		if (ch == '\n') ch = 0xB6;
		
		glyph g = font->glyphs[ch];
		
		s32 y_ = y + font->px_h + g.yoff;
		s32 x_to_render = x_ + (g.lsb);
		
		if (global_use_gpu)
		{
			glBindTexture(GL_TEXTURE_2D, g.textureID);
			glBegin(GL_QUADS);
			
			s32 width = g.width;
			
			glTexCoord2i(0, 0); glVertex3i(x_to_render,y_, render_depth);
			glTexCoord2i(0, 1); glVertex3i(x_to_render,y_+g.height, render_depth);
			glTexCoord2i(1, 1); glVertex3i(x_to_render+g.width,y_+g.height, render_depth);
			glTexCoord2i(1, 0); glVertex3i(x_to_render+g.width,y_, render_depth);
			
			glEnd();
		}
		else
		{
			render_target rec = _get_actual_rect(x_to_render, y_, g.width, g.height);
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				for (s32 x = rec.x; x < rec.w; x++)
				{
					_copy_glyph_pixel(x, y, &g, rec, tint);
				}
			}
		}
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x_ += (g.advance);
		
		index++;
		if (index == selection_start)
		{
			selection_start_x = x_;
		}
		if (index == selection_start+selection_length)
		{
			selection_end_x = x_;
		}
	}
	
	if (global_use_gpu)
		glDisable(GL_TEXTURE_2D);
	
	render_rectangle(selection_start_x, y-3, selection_end_x-selection_start_x, TEXTBOX_HEIGHT - 10, rgba(66, 134, 244, 120));
	
	return x_ - x;
}

s32 render_text_with_cursor(font *font, s32 x, s32 y, char *text, color tint, s32 cursor_pos)
{
	if (!font->loaded)
		return 0;
	
	if (global_use_gpu)
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	}
	
	float x_ = x;
	utf8_int32_t ch;
	s32 index = 0;
	s32 cursor_x = x_;
	while((text = utf8codepoint(text, &ch)) && ch)
	{
		if (ch == 9) ch = 32;
		utf8_int32_t ch_next;
		utf8codepoint(text, &ch_next);
		if (ch < TEXT_CHARSET_START || ch > TEXT_CHARSET_END) 
		{
			ch = 0x3f;
		}
		if (ch == '\n') ch = 0xB6;
		
		glyph g = font->glyphs[ch];
		
		s32 y_ = y + font->px_h + g.yoff;
		s32 x_to_render = x_ + (g.lsb);
		
		if (global_use_gpu)
		{
			glBindTexture(GL_TEXTURE_2D, g.textureID);
			glBegin(GL_QUADS);
			
			glTexCoord2i(0, 0); glVertex3i(x_to_render,y_, render_depth);
			glTexCoord2i(0, 1); glVertex3i(x_to_render,y_+g.height, render_depth);
			glTexCoord2i(1, 1); glVertex3i(x_to_render+g.width,y_+g.height, render_depth);
			glTexCoord2i(1, 0); glVertex3i(x_to_render+g.width,y_, render_depth);
			
			glEnd();
		}
		else
		{
			render_target rec = _get_actual_rect(x_to_render, y_, g.width, g.height);
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				for (s32 x = rec.x; x < rec.w; x++)
				{
					_copy_glyph_pixel(x, y, &g, rec, tint);
				}
			}
		}
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x_ += (g.advance);
		index++;
		
		if (index == cursor_pos)
		{
			cursor_x = x_;
		}
	}
	
	if (global_use_gpu)
		glDisable(GL_TEXTURE_2D);
	
	render_rectangle(cursor_x, y-3, 2, TEXTBOX_HEIGHT - 10, global_ui_context.style.textbox_foreground);
	
	return x_ - x;
}

s32 render_text(font *font, s32 x, s32 y, char *text, color tint)
{
	if (!font->loaded)
		return 0;
	
	if (global_use_gpu)
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	}
	
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
		if (ch == 10) ch = 0xB6;
		if (ch == 13) continue;
		
		glyph g = font->glyphs[ch];
		
		s32 y_ = y + font->px_h + g.yoff;
		s32 x_to_render = x_ + (g.lsb);
		
		if (global_use_gpu)
		{
			glBindTexture(GL_TEXTURE_2D, g.textureID);
			glBegin(GL_QUADS);
			
			glTexCoord2i(0, 0); glVertex3i(x_to_render,y_, render_depth);
			glTexCoord2i(0, 1); glVertex3i(x_to_render,y_+g.height, render_depth);
			glTexCoord2i(1, 1); glVertex3i(x_to_render+g.width,y_+g.height, render_depth);
			glTexCoord2i(1, 0); glVertex3i(x_to_render+g.width,y_, render_depth);
			
			glEnd();
		}
		else
		{
			render_target rec = _get_actual_rect(x_to_render, y_, g.width, g.height);
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				for (s32 x = rec.x; x < rec.w; x++)
				{
					_copy_glyph_pixel(x, y, &g, rec, tint);
				}
			}
		}
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x_ += (g.advance);
	}
	
	if (global_use_gpu)
		glDisable(GL_TEXTURE_2D);
	
	return x_ - x;
}

s32 render_text_cutoff(font *font, s32 x, s32 y, char *text, color tint, u16 cutoff_width)
{
	if (!font->loaded)
		return 0;
	
	if (global_use_gpu)
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	}
	
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
		
		s32 y__ = y_ + font->px_h + g.yoff;
		s32 x_to_render = x_ + (g.lsb);
		
		if (global_use_gpu)
		{
			glBindTexture(GL_TEXTURE_2D, g.textureID);
			glBegin(GL_QUADS);
			
			glTexCoord2i(0, 0); glVertex3i(x_to_render,y__, render_depth);
			glTexCoord2i(0, 1); glVertex3i(x_to_render,y__+g.height, render_depth);
			glTexCoord2i(1, 1); glVertex3i(x_to_render+g.width,y__+g.height, render_depth);
			glTexCoord2i(1, 0); glVertex3i(x_to_render+g.width,y__, render_depth);
			
			glEnd();
		}
		else
		{
			render_target rec = _get_actual_rect(x_to_render, y__, g.width, g.height);
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				for (s32 x = rec.x; x < rec.w; x++)
				{
					_copy_glyph_pixel(x, y, &g, rec, tint);
				}
			}
		}
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x_ += (g.advance);
		
		if (x_ > x+cutoff_width)
		{
			x_ = x;
			y_ += font->size;
			is_new_line = true;
		}
	}
	
	if (global_use_gpu)
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
		if (ch == '\n') ch = 0xB6;
		
		glyph g = font->glyphs[ch];
		
		s32 width = g.width;
		s32 width_next = font->glyphs[ch_next].width;
		
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x += (g.advance);
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
		if (ch == '\n') ch = 0xB6;
		
		glyph g = font->glyphs[ch];
		s32 width = g.width;
		
		if (i >= from)
		{
			/* add kerning */
			//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
			x += (g.advance);
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
		if (ch == '\n') ch = 0xB6;
		
		glyph g = font->glyphs[ch];
		s32 width = g.width;
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x += (g.advance);
		
		i++;
	}
	
	return x;
}

s32 calculate_text_width(font *font, char *text)
{
	if (!font->loaded)
		return 0;
	
	float x = 0;
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
		if (ch == '\n') ch = 0xB6;
		
		glyph g = font->glyphs[ch];
		s32 width = g.width;
		
		/* add kerning */
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		x += (g.advance);
	}
	
	return x;
}

void render_triangle(s32 x, s32 y, s32 w, s32 h, color tint, triangle_direction dir)
{
	if (global_use_gpu)
	{
		glBegin(GL_TRIANGLES);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
		
		if (dir == TRIANGLE_DOWN)
		{
			glVertex3i(x+(w/2), y+h, render_depth);
			glVertex3i(x, y, render_depth);
			glVertex3i(x+w, y, render_depth);
		}
		else if (dir == TRIANGLE_UP)
		{
			glVertex3i(x+(w/2), y, render_depth);
			glVertex3i(x+w, y+h, render_depth);
			glVertex3i(x, y+h, render_depth);
		}
		else if (dir == TRIANGLE_LEFT)
		{
			glVertex3i(x, y+(w/2), render_depth);
			glVertex3i(x+h, y, render_depth);
			glVertex3i(x+h, y+w, render_depth);
		}
		else if (dir == TRIANGLE_RIGHT)
		{
			assert(0 && "not implemented"); // TODO(Aldrik): implement
		}
		
		glEnd();
	}
	else
	{
		render_target rec = _get_actual_rect(x,y,w,h);
		
		s32 ac_w = rec.w - rec.x;
		
		if (dir == TRIANGLE_DOWN)
		{
			for (s32 y = rec.y; y < rec.h; y++)
			{
				s32 _y = y-rec.y;
				
				for (s32 x = rec.x; x < rec.w; x++)
				{
					s32 _x = x-rec.x;
					if (_x < _y / 2 || _x >= ac_w-(_y/2)-1) continue;
					
					_set_pixel(x, y, tint);
				}
			}
		}
		else if (dir == TRIANGLE_UP)
		{
			s32 ac_h = rec.h - rec.y;
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				s32 _y = (y-rec.y);
				_y = ac_h - _y;
				
				for (s32 x = rec.x; x < rec.w; x++)
				{
					s32 _x = x-rec.x;
					if (_x < _y / 2 || _x >= ac_w-(_y/2)) continue;
					
					_set_pixel(x, y, tint);
				}
			}
		}
		else if (dir == TRIANGLE_LEFT)
		{
			s32 ac_h = rec.h - rec.y;
			
			for (s32 y = rec.y; y < rec.h; y++)
			{
				s32 _y = (y-rec.y);
				_y = ac_h - _y;
				
				for (s32 x = rec.x; x < rec.w; x++)
				{
					s32 _x = x-rec.x;
					_x = ac_w - _x;
					if (_x / 2 > _y-1 || _x / 2 >= ac_w-(_y)) continue;
					
					_set_pixel(x, y, tint);
				}
			}
		}
		else if (dir == TRIANGLE_RIGHT)
		{
			assert(0 && "not implemented"); // TODO(Aldrik): implement
		}
		
	}
}

void render_rectangle(s32 x, s32 y, s32 width, s32 height, color tint)
{
	if (global_use_gpu)
	{
		glBegin(GL_QUADS);
		glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
		glVertex3i(x, y, render_depth);
		glVertex3i(x, y+height, render_depth);
		glVertex3i(x+width, y+height, render_depth);
		glVertex3i(x+width, y, render_depth);
		glEnd();
	}
	else
	{
		render_target rec = _get_actual_rect(x,y,width,height);
		
		for (s32 y = rec.y; y < rec.h; y++)
		{
			for (s32 x = rec.x; x < rec.w; x++)
			{
				_set_pixel(x, y, tint);
			}
		}
	}
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
	if (global_use_gpu)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(x-1, window->height-h-y-1, w+1, h+1);
	}
	else
	{
		current_scissor = (vec4){x,y,w+1,h+1};
	}
}

vec4 render_get_scissor(platform_window *window)
{
	if (global_use_gpu)
	{
		vec4 vec;
		glGetIntegerv(GL_SCISSOR_BOX, (GLint*)(&vec));
		vec.x += 1;
		vec.w -= 1;
		vec.h -= 1;
		vec.y += 1;
		return vec;
	}
	else
	{
		return current_scissor;
	}
}

void render_reset_scissor()
{
	if (global_use_gpu)
	{
		glDisable(GL_SCISSOR_TEST);
	}
	else
	{
		current_scissor = (vec4){0,0,drawing_window->width+1,drawing_window->height+1};
	}
}
