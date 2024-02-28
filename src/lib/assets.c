/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
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
	asset_collection.done_loading_assets = false;
	
	global_asset_collection = asset_collection;
}

inline static bool is_big_endian()
{
	volatile uint32_t i=0x01234567;
    // return 1 for big endian, 0 for little endian.
    return !((*((uint8_t*)(&i))) == 0x67);
}

void assets_stop_if_done()
{
	if (global_asset_collection.queue.queue.length == 0 && !global_asset_collection.done_loading_assets)
	{
		global_asset_collection.done_loading_assets = true;
		
#ifdef MODE_TIMESTARTUP
		abort();
#endif
		
#if defined(MODE_DEBUGMEM) && !defined(MODE_TEST)
		printf("allocated at startup: %dkb\n", __total_allocated/1000);
		printf("reallocated at startup: %dkb\n", __total_reallocated/1000);
#endif
		
#if defined(MODE_DEVELOPER) && !defined(MODE_TEST)
		printf("frames drawn with missing assets: %d\n", __frames_drawn_with_missing_assets);
#endif
	}
	
#ifdef MODE_DEVELOPER
	if (global_asset_collection.queue.queue.length != 0 && !global_asset_collection.done_loading_assets)
	{
		__frames_drawn_with_missing_assets++;
	}
#endif
}

bool assets_do_post_process()
{
	assets_stop_if_done();
	
	bool result = false;
	
	mutex_lock(&asset_mutex);
	
	for (int i = 0; i < global_asset_collection.post_process_queue.length; i++)
	{
		asset_task *task = array_at(&global_asset_collection.post_process_queue, i);
		
		if (task->type == ASSET_IMAGE || task->type == ASSET_BITMAP)
		{
			if (task->image->data && task->valid)
			{
				if (!global_use_gpu) { task->image->loaded = true; goto done; }
				
				glGenTextures(1, &task->image->textureID);
				glBindTexture(GL_TEXTURE_2D, task->image->textureID);
				
				s32 flag = is_big_endian() ? GL_UNSIGNED_INT_8_8_8_8 : 
				GL_UNSIGNED_INT_8_8_8_8_REV;
				
				if (task->type == ASSET_IMAGE)
					glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, task->image->width, 
								 task->image->height, 0,  GL_RGBA, flag, task->image->data);
				else
					glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, task->image->width, 
								 task->image->height, 0,  GL_BGRA, flag, task->image->data);
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				task->image->loaded = true;
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		else if (task->type == ASSET_FONT)
		{
			if (task->valid)
			{
				if (!global_use_gpu) { task->font->loaded = true; goto done; }
				
				for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
				{
					glyph *g = &task->font->glyphs[i];
					
					glGenTextures(1, &g->textureID);
					glBindTexture(GL_TEXTURE_2D, g->textureID);
					
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, g->width,g->height,
								 0, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap );
				}
				
				task->font->loaded = true;
			}
		}
		
		done:
		result = true;
		array_remove_at(&global_asset_collection.post_process_queue, i);
	}
	
	mutex_unlock(&asset_mutex);
	
	return result;
}

bool assets_queue_worker_load_bitmap(image *image)
{
#ifdef MODE_DEVELOPER
	u64 stamp = platform_get_time(TIME_FULL, TIME_US);
#endif
	
#pragma pack(push, 1)
	typedef struct {
		unsigned short type;                 /* Magic identifier            */
		unsigned int size;                       /* File size in bytes          */
		unsigned int reserved;
		unsigned int offset;                     /* Offset to image data, bytes */
	} HEADER;
	typedef struct {
		unsigned int size;               /* Header size in bytes      */
		int width,height;                /* Width and height of image */
		unsigned short planes;       /* Number of colour planes   */
		unsigned short bits;         /* Bits per pixel            */
		unsigned int compression;        /* Compression type          */
		unsigned int imagesize;          /* Image size in bytes       */
		int xresolution,yresolution;     /* Pixels per meter          */
		unsigned int ncolours;           /* Number of colours         */
		unsigned int importantcolours;   /* Important colours         */
	} INFOHEADER;
#pragma pack(pop)
	
	HEADER* header = (HEADER*)image->start_addr;
	INFOHEADER* info = (INFOHEADER*)(image->start_addr+sizeof(HEADER));
	
	image->data = image->start_addr+header->offset;
	image->width = info->width;
	image->height = info->height;
	image->channels = info->bits/8;
	
	debug_print_elapsed(stamp, "loaded bitmap in");
	
	return image->data != 0;
}

bool assets_queue_worker_load_image(image *image)
{
#ifdef MODE_DEVELOPER
	u64 stamp = platform_get_time(TIME_FULL, TIME_US);
#endif
	
	//stbi_convert_iphone_png_to_rgb(0);
	image->data = stbi_load_from_memory(image->start_addr,
										image->end_addr - image->start_addr,
										&image->width,
										&image->height,
										&image->channels,
										STBI_rgb_alpha);
	
	debug_print_elapsed(stamp, "loaded image in");
	
	return image->data != 0;
}

bool assets_queue_worker_load_font(font *font)
{
#ifdef MODE_DEVELOPER
	u64 stamp = platform_get_time(TIME_FULL, TIME_US);
#endif
	
	unsigned char *ttf_buffer = (unsigned char*)font->start_addr;
	
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0)))
    {
		return false;
	}
	float scale = stbtt_ScaleForPixelHeight(&info, font->size);
	
	for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
	{
		s32 w, h, xoff, yoff;
		
		glyph new_glyph;
		new_glyph.bitmap = stbtt_GetCodepointBitmap(&info, 0, scale, i, &w, &h, &xoff, &yoff);
		new_glyph.width = w;
		new_glyph.height = h;
		new_glyph.xoff = xoff;
		new_glyph.yoff = yoff;
		
		stbtt_GetCodepointHMetrics(&info, i, &new_glyph.advance, &new_glyph.lsb);
		new_glyph.advance *= scale;
		new_glyph.lsb *= scale;
		
		if (i == 'M') font->px_h = -yoff;
		
		font->glyphs[i-TEXT_CHARSET_START] = new_glyph;
	}
	
	font->info = info;
	font->scale = scale;
	
	debug_print_elapsed(stamp, "loaded font in");
	
	return true;
}

void *assets_queue_worker()
{
	while (global_asset_collection.valid && !global_asset_collection.done_loading_assets)
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
			if (buf.type == ASSET_BITMAP)
			{
				bool result = assets_queue_worker_load_bitmap(buf.image);
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
		
		thread_sleep(1000);
	}
	
	thread_exit();
	
	return 0;
}

image *assets_load_image(u8 *start_addr, u8 *end_addr)
{
	// check if image is already loaded or loading
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img_at = array_at(&global_asset_collection.images, i);
		
		if (start_addr == img_at->start_addr && img_at->references > 0)
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
		if (global_use_gpu)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			glDeleteTextures(1, &image_to_destroy->textureID);
		}
		
		image_to_destroy->references = 0;
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
		
		if (start_addr == font_at->start_addr && font_at->size == size && font_at->references > 0)
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
		if (global_use_gpu)
		{
			for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
			{
				glyph g = font_to_destroy->glyphs[i];
				glBindTexture(GL_TEXTURE_2D, 0);
				glDeleteTextures(1, &g.textureID);
			}
		}
		
		font_to_destroy->references = 0;
	}
	else
	{
		font_to_destroy->references--;
	}
}

void assets_destroy()
{
	global_asset_collection.valid = false;
	global_asset_collection.done_loading_assets = false;
	
	array_destroy(&global_asset_collection.images);
	array_destroy(&global_asset_collection.fonts);
	
	array_destroy(&global_asset_collection.queue.queue);
	array_destroy(&global_asset_collection.post_process_queue);
	
	mem_free(binary_path);
	
	mutex_destroy(&asset_mutex);
}

image *assets_load_bitmap(u8 *start_addr, u8 *end_addr)
{
	// check if image is already loaded or loading
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img_at = array_at(&global_asset_collection.images, i);
		
		if (start_addr == img_at->start_addr && img_at->references > 0)
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
	
	// NOTE(Aldrik): we should never realloc the image array because pointers will be 
	// invalidated.
	assert(global_asset_collection.images.reserved_length > global_asset_collection.images.length);
	
	int index = array_push(&global_asset_collection.images, &new_image);
	
	asset_task task;
	task.type = ASSET_BITMAP;
	task.image = array_at(&global_asset_collection.images, index);
	
	mutex_lock(&asset_mutex);
	array_push(&global_asset_collection.queue.queue, &task);
	mutex_unlock(&asset_mutex);
	
	return task.image;
}

void assets_destroy_bitmap(image *image_to_destroy)
{
	if (image_to_destroy->references == 1)
	{
		if (global_use_gpu)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			glDeleteTextures(1, &image_to_destroy->textureID);
		}
		
		image_to_destroy->references = 0;
	}
	else
	{
		image_to_destroy->references--;
	}
}

void assets_switch_render_method()
{
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img_at = array_at(&global_asset_collection.images, i);
		
		if (global_use_gpu)
		{
			asset_task task;
			task.type = ASSET_IMAGE;
			task.image = img_at;
			task.valid = true;
			
			array_push(&global_asset_collection.post_process_queue, &task);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			glDeleteTextures(1, &img_at->textureID);
		}
	}
	
	for (int i = 0; i < global_asset_collection.fonts.length; i++)
	{
		font *font_at = array_at(&global_asset_collection.fonts, i);
		
		if (global_use_gpu)
		{
			asset_task task;
			task.type = ASSET_FONT;
			task.font = font_at;
			task.valid = true;
			
			array_push(&global_asset_collection.post_process_queue, &task);
		}
		else
		{
			for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
			{
				glyph g = font_at->glyphs[i];
				glBindTexture(GL_TEXTURE_2D, 0);
				glDeleteTextures(1, &g.textureID);
			}
		}
	}
}
