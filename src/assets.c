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

void assets_create()
{
	assets asset_collection;
	asset_collection.images = array_create(sizeof(image));
	asset_collection.fonts = array_create(sizeof(font));
	
	array_reserve(&asset_collection.images, ASSET_IMAGE_COUNT);
	array_reserve(&asset_collection.fonts, ASSET_FONT_COUNT);
	
	asset_collection.queue.queue = array_create(sizeof(asset_task));
	asset_collection.post_process_queue = array_create(sizeof(asset_task));
	
	array_reserve(&asset_collection.queue.queue, ASSET_QUEUE_COUNT);
	array_reserve(&asset_collection.post_process_queue, ASSET_QUEUE_COUNT);
	
	asset_mutex = mutex_create();
	asset_collection.valid = true;
	
	global_asset_collection = asset_collection;
}

inline static bool is_big_endian()
{
	volatile uint32_t i=0x01234567;
    // return 1 for big endian, 0 for little endian.
    return !((*((uint8_t*)(&i))) == 0x67);
}

void assets_do_post_process()
{
	mutex_lock(&asset_mutex);
	
	for (int i = 0; i < global_asset_collection.post_process_queue.length; i++)
	{
		asset_task *task = array_at(&global_asset_collection.post_process_queue, i);
		
		if (task->type == ASSET_IMAGE)
		{
			if (task->image->data && task->valid)
			{
				glGenTextures(1, &task->image->textureID);
				// "Bind" the newly created texture : all future texture functions will modify this texture
				glBindTexture(GL_TEXTURE_2D, task->image->textureID);
				
				s32 flag = is_big_endian() ? GL_UNSIGNED_INT_8_8_8_8 : 
				GL_UNSIGNED_INT_8_8_8_8_REV;
				
				// Give the image to OpenGL
				glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, task->image->width, 
							 task->image->height, 0,  GL_RGBA, flag, task->image->data);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				task->image->loaded = true;
				
				if (!task->image->keep_in_memory)
					stbi_image_free(task->image->data);
			}
		}
		else if (task->type == ASSET_FONT)
		{
			if (task->font->bitmap && task->valid)
			{
				glGenTextures(1, &task->font->textureID);
				glBindTexture(GL_TEXTURE_2D, task->font->textureID);
				
				glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, task->font->palette_width,task->font->palette_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, task->font->bitmap);
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				task->font->loaded = true;
			}
		}
		
		array_remove_at(&global_asset_collection.post_process_queue, i);
	}
	
	mutex_unlock(&asset_mutex);
}

bool assets_queue_worker_load_image(image *image)
{
	set_active_directory(binary_path);
	
	image->data = stbi_load_from_memory(image->start_addr,
										image->end_addr - image->start_addr,
										&image->width,
										&image->height,
										&image->channels,
										STBI_rgb_alpha);
	
	return !(image->data == 0);
}

bool assets_queue_worker_load_font(font *font)
{
	font->bitmap = 0;
	
	unsigned char *ttf_buffer = (unsigned char*)font->start_addr;
	
	/* prepare font */
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0)))
    {
		return false;
	}
	
	int b_w = (TEXT_CHARSET_END-TEXT_CHARSET_START+1)*(font->size*2); /* bitmap width */
    int b_h = font->size*2; /* bitmap height */
	int l_h = font->size*2; /* line height */
	
	font->palette_width = b_w;
	font->palette_height = b_h;
	
	/* create a bitmap for the phrase */
    unsigned char* bitmap = mem_alloc(b_w * b_h);
	memset(bitmap, 0, b_w * b_h);
	
	/* calculate font scaling */
	float scale = stbtt_ScaleForPixelHeight(&info, l_h);
	
	int x = 0;
	
    int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
	
	ascent *= scale;
	descent *= scale;
	
    for (int i = TEXT_CHARSET_START; i <= TEXT_CHARSET_END; ++i)
    {
		if (!stbtt_FindGlyphIndex(&info, i)) continue;
		
        /* get bounding box for character (may be offset to account for chars that dip above or below the line */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
        
        /* compute y (different characters have different heights */
        int y = ascent + c_y1;
        
        /* render character (stride and offset is important here) */
        int byteOffset = x + (y  * b_w);
        stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, i);
        
        /* how wide is this character */
        int ax;
        stbtt_GetCodepointHMetrics(&info, i, &ax, 0);
        x += font->size*2;
        
		font->glyph_widths[i-TEXT_CHARSET_START] = (ax*scale);
	}
	
	font->info = info;
	font->scale = scale;
	
	font->bitmap = bitmap;
	
	return true;
}

void *assets_queue_worker()
{
	while (global_asset_collection.valid)
	{
		if (mutex_trylock(&asset_mutex))
		{
			int queue_length = global_asset_collection.queue.queue.length;
			if (!queue_length) 
			{
				mutex_unlock(&asset_mutex);
				continue;
			}
			
			asset_task *task = array_at(&global_asset_collection.queue.queue, 0);
			asset_task buf = *task;
			array_remove_at(&global_asset_collection.queue.queue, 0);
			mutex_unlock(&asset_mutex);
			
			// load here
			if (buf.type == ASSET_IMAGE)
			{
				bool result = assets_queue_worker_load_image(buf.image);
				buf.valid = result;
			}
			else if (buf.type == ASSET_FONT)
			{
				bool result = assets_queue_worker_load_font(buf.font);
				buf.valid = result;
			}
			
			mutex_lock(&asset_mutex);
			
			assert(global_asset_collection.post_process_queue.reserved_length > 
				   global_asset_collection.post_process_queue.length);
			
			array_push(&global_asset_collection.post_process_queue, &buf);
			mutex_unlock(&asset_mutex);
		}
		
		// 3 ms
		thread_sleep(3000);
	}
	
	return 0;
}

image *assets_load_image(u8 *start_addr, u8 *end_addr, bool keep_in_memory)
{
	// check if image is already loaded or loading
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img_at = array_at(&global_asset_collection.images, i);
		
		if (start_addr == img_at->start_addr)
		{
			// image is already loaded/loading
			img_at->references++;
			return img_at;
		}
	}
	
	image new_image;
	new_image.loaded = false;
	new_image.start_addr = start_addr;
	new_image.end_addr = end_addr;
	new_image.references = 1;
	new_image.keep_in_memory = keep_in_memory;
	
	// NOTE(Aldrik): we should never realloc the image array because pointers will be 
	// invalidated.
	assert(global_asset_collection.images.reserved_length > global_asset_collection.images.length);
	
	int index = array_push(&global_asset_collection.images, &new_image);
	
	asset_task task;
	task.type = ASSET_IMAGE;
	task.image = array_at(&global_asset_collection.images, index);
	
	mutex_lock(&asset_mutex);
	array_push(&global_asset_collection.queue.queue, &task);
	mutex_unlock(&asset_mutex);
	
	return task.image;
}

void assets_destroy_image(image *image_to_destroy)
{
	if (image_to_destroy->references == 1)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &image_to_destroy->textureID);
		
		if (image_to_destroy->keep_in_memory)
			stbi_image_free(image_to_destroy->data);
		
		//array_remove(&global_asset_collection.images, image_at);
	}
	else
	{
		image_to_destroy->references--;
	}
}

font *assets_load_font(u8 *start_addr, u8 *end_addr, s16 size)
{
	//assert(!(size % 4));
	for (int i = 0; i < global_asset_collection.fonts.length; i++)
	{
		font *font_at = array_at(&global_asset_collection.fonts, i);
		
		if (start_addr == font_at->start_addr && font_at->size == size)
		{
			// font is already loaded/loading
			font_at->references++;
			return font_at;
		}
	}
	
	font new_font;
	new_font.loaded = false;
	new_font.start_addr = start_addr;
	new_font.end_addr = end_addr;
	new_font.size = size;
	new_font.references = 1;
	
	//new_font.loaded = true;
	
	// NOTE(Aldrik): we should never realloc the font array because pointers will be 
	// invalidated.
	assert(global_asset_collection.fonts.reserved_length > global_asset_collection.fonts.length);
	
	int index = array_push(&global_asset_collection.fonts, &new_font);
	
	asset_task task;
	task.type = ASSET_FONT;
	task.font = array_at(&global_asset_collection.fonts, index);
	
	mutex_lock(&asset_mutex);
	array_push(&global_asset_collection.queue.queue, &task);
	mutex_unlock(&asset_mutex);
	
	return task.font;
}

void assets_destroy_font(font *font_to_destroy)
{
	if (font_to_destroy->references == 1)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &font_to_destroy->textureID);
		
		mem_free(font_to_destroy->bitmap);
		//array_remove(&global_asset_collection.fonts, font_at);
	}
	else
	{
		font_to_destroy->references--;
	}
}

void assets_destroy()
{
	global_asset_collection.valid = false;
	
	array_destroy(&global_asset_collection.images);
	array_destroy(&global_asset_collection.fonts);
	
	array_destroy(&global_asset_collection.queue.queue);
	array_destroy(&global_asset_collection.post_process_queue);
	
	mem_free(binary_path);
	
	mutex_destroy(&asset_mutex);
}
