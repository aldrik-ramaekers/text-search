
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
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
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
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
}

void render_image_tint(image *image, s32 x, s32 y, s32 width, s32 height, color tint)
{
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
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
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
}

void render_font_palette(font *font, s32 x, s32 y, s32 w, s32 h, color tint)
{
	if (!font->loaded)
		return;
	
	glBindTexture(GL_TEXTURE_2D, font->textureID);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	
	glTexCoord2i(0, 0); glVertex2i(x, y);
	glTexCoord2i(0, 1); glVertex2i(x, y+h);
	glTexCoord2i(1, 1); glVertex2i(x+w, y+h);
	glTexCoord2i(1, 0); glVertex2i(x+w, y);
	
	glEnd();
	
	glFlush();
	
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

s32 render_text(font *font, s32 x, s32 y, char *text, color tint)
{
	if (!font->loaded)
		return 0;
	
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
	glBindTexture(GL_TEXTURE_2D, font->textureID);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	
	s32 x_ = x;
	while(*text)
	{
		char ch = *text;
		char ch_next = *(text+1);
		s32 offsetx = (font->size*2)*(ch-32);
		
		float ipw = 1.0f / font->palette_width, iph = 1.0f / font->palette_height;
		
		float sx0, sy0, sx1, sy1;
		sx0 = ipw*offsetx;
		sy0 = 0;
		sx1 = ipw*(offsetx+font->size*2);
		sy1 = iph*font->size*2;
		
		s32 width = font->glyph_widths[ch-32];
		glTexCoord2f(sx0,sy0); glVertex3i(x_,y, render_depth);
		glTexCoord2f(sx0,sy1); glVertex3i(x_,y+font->size, render_depth);
		glTexCoord2f(sx1,sy1); glVertex3i(x_+font->size,y+font->size, render_depth);
		glTexCoord2f(sx1,sy0); glVertex3i(x_+font->size,y, render_depth);
		
		/* add kerning */
        //int kern = 0;
		
		//if (ch_next != 0)
		//{
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		
		//if (kern != 0) printf("%d\n", kern);
		//}
		
		if (ch != '.' || ch != ',')
			x_+=width/2;//+(kern*font->scale);
		else
			x_+=font->size/4;
		
		++text;
	}
	
	glEnd();
	
	glFlush();
	
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
	
	return x_ - x;
}

s32 render_text_vertical(font *font, s32 x, s32 y, char *text, color tint)
{
	if (!font->loaded)
		return 0;
	
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
	glBindTexture(GL_TEXTURE_2D, font->textureID);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	
	s32 x_ = x-(font->size/4);
	s32 y_ = y;
	while(*text)
	{
		char ch = *text;
		char ch_next = *(text+1);
		s32 offsetx = (font->size*2)*(ch-32);
		
		float ipw = 1.0f / font->palette_width, iph = 1.0f / font->palette_height;
		
		float sx0, sy0, sx1, sy1;
		sx0 = ipw*offsetx;
		sy0 = 0;
		sx1 = ipw*(offsetx+font->size*2);
		sy1 = iph*font->size*2;
		
		s32 width = font->glyph_widths[ch-32];
		
		glTexCoord2f(sx0,sy0); glVertex3i(x_+font->size,y_, render_depth);
		glTexCoord2f(sx0,sy1); glVertex3i(x_,y_, render_depth);
		glTexCoord2f(sx1,sy1); glVertex3i(x_,y_+font->size, render_depth);
		glTexCoord2f(sx1,sy0); glVertex3i(x_+font->size,y_+font->size, render_depth);
		
		y_+=width/2;
		
		++text;
	}
	
	glEnd();
	
	glFlush();
	
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
	
	return y_ -y;
}

s32 render_text_cutoff(font *font, s32 x, s32 y, char *text, color tint, u16 cutoff_width)
{
	if (!font->loaded)
		return 0;
	
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
	glBindTexture(GL_TEXTURE_2D, font->textureID);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	//glTexCoord2i(0, 0); glVertex2i(x, y);
	//glTexCoord2i(0, 1); glVertex2i(x, y+font->palette_height);
	//glTexCoord2i(1, 1); glVertex2i(x+font->palette_width, y+font->palette_height);
	//glTexCoord2i(1, 0); glVertex2i(x+font->palette_width, y);
	
	s32 x_ = x;
	s32 y_ = y + font->size;
	while(*text)
	{
		char ch = *text;
		char ch_next = *(text+1);
		s32 offsetx = (font->size*2)*(ch-32);
		
		if (ch == '\n')
		{
			x_ = x;
			y_ += font->size;
			++text;
			continue;
		}
		
		float ipw = 1.0f / font->palette_width, iph = 1.0f / font->palette_height;
		
		float sx0, sy0, sx1, sy1;
		sx0 = ipw*offsetx;
		sy0 = 0;
		sx1 = ipw*(offsetx+font->size*2);
		sy1 = iph*font->size*2;
		
		s32 width = font->glyph_widths[ch-32];
		
		glTexCoord2f(sx0,sy0); glVertex3i(x_,y_, render_depth);
		glTexCoord2f(sx0,sy1); glVertex3i(x_,y_+font->size, render_depth);
		glTexCoord2f(sx1,sy1); glVertex3i(x_+font->size,y_+font->size, render_depth);
		glTexCoord2f(sx1,sy0); glVertex3i(x_+font->size,y_, render_depth);
		
		/* add kerning */
        //int kern = 0;
		
		//if (ch_next != 0)
		//{
		//kern = stbtt_GetCodepointKernAdvance(&font->info, ch, ch_next);
		
		//if (kern != 0) printf("%d\n", kern);
		//}
		
		if (ch != '.' || ch != ',')
			x_+=width / 2;//+(kern*font->scale);
		else
			x_+=font->size/4;
		
		if (x_ > x+cutoff_width)
		{
			x_ = x;
			y_ += font->size;
		}
		
		++text;
	}
	
	glEnd();
	
	glFlush();
	
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
	
	return y_ - y;
	
}

s32 calculate_text_width(font *font, char *text)
{
	if (!font->loaded)
		return 0;
	
	s32 x = 0;
	while(*text)
	{
		char ch = *text;
		char ch_next = *(text+1);
		s32 width = font->glyph_widths[ch-32];
		
		if (ch != '.' || ch != ',')
			x+=width / 2;//+(kern*font->scale);
		else
			x+=font->size/4;
		
		++text;
	}
	
	return x;
}

s32 calculate_text_height(font *font, s32 cutoff_width, char *text)
{
	if (!font->loaded)
		return 0;
	
	s32 x_ = 0;
	s32 y = font->size;
	while(*text)
	{
		char ch = *text;
		char ch_next = *(text+1);
		s32 width = font->glyph_widths[ch-32];
		
		x_ += width / 2;
		
		if (x_ > cutoff_width)
		{
			x_ = 0;
			y += font->size;
		}
		
		++text;
	}
	
	return y;
}

void render_triangle(s32 x, s32 y, s32 w, s32 h, color tint)
{
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
	glBegin(GL_TRIANGLES);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	glVertex3i(x+(w/2), y+h, render_depth);
	glVertex3i(x, y, render_depth);
	glVertex3i(x+w, y, render_depth);
	glEnd();
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
}

void render_rectangle(s32 x, s32 y, s32 width, s32 height, color tint)
{
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
	glBegin(GL_QUADS);
	glColor4f(tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f); 
	glVertex3i(x, y, render_depth);
	glVertex3i(x, y+height, render_depth);
	glVertex3i(x+width, y+height, render_depth);
	glVertex3i(x+width, y, render_depth);
	glEnd();
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
}

void render_rectangle_tint(s32 x, s32 y, s32 width, s32 height, color tint[4])
{
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
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
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
}

void render_rectangle_outline(s32 x, s32 y, s32 width, s32 height, u16 outline_w, color tint)
{
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
	// left
	render_rectangle(x, y, outline_w, height, tint);
	// right
	render_rectangle(x+width-outline_w, y, outline_w, height, tint);
	// top
	render_rectangle(x+outline_w, y, width-(outline_w*2), outline_w, tint);
	// bottom
	render_rectangle(x+outline_w, y+height-outline_w, width-(outline_w*2), outline_w, tint);
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
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