
void assets_create()
{
	char active_dir_buf[MAX_INPUT_LENGTH];
	get_active_directory(active_dir_buf);
	binary_path = platform_get_full_path(active_dir_buf);
	
	
	assets asset_collection;
	asset_collection.images = array_create(sizeof(image));
	asset_collection.fonts = array_create(sizeof(font));
	asset_collection.samples = array_create(sizeof(sample));
	
	array_reserve(&asset_collection.images, ASSET_IMAGE_COUNT);
	array_reserve(&asset_collection.fonts, ASSET_FONT_COUNT);
	array_reserve(&asset_collection.samples, ASSET_SAMPLE_COUNT);
	
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
	
#ifdef MODE_DEVELOPER
	profiler_begin(profiler_start);
#endif
	
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
				
				// TODO: should we keep the memory to create sub images?
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
				free(task->font->bitmap);
			}
		}
		
		array_remove_at(&global_asset_collection.post_process_queue, i);
	}
	
	
#ifdef MODE_DEVELOPER
	profiler_end(profiler_start);
#endif
	
	mutex_unlock(&asset_mutex);
}

bool assets_queue_worker_load_image(image *image)
{
	set_active_directory(binary_path);
	
	image->data = stbi_load(image->path,
							&image->width,
							&image->height,
							&image->channels,
							STBI_rgb_alpha);
	
	return !(image->data == 0);
}

bool assets_queue_worker_load_font(font *font)
{
	set_active_directory(binary_path);
	
	font->bitmap = 0;
	file_content content = platform_read_file_content(font->path, "rb");
	if (!content.content) return false;
	
	unsigned char *ttf_buffer = content.content;
	
	/* prepare font */
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf_buffer, 0))
    {
		free(ttf_buffer);
		return false;
	}
	
	// ascii 32 - 126
	int b_w = 95*(font->size*2); /* bitmap width */
    int b_h = font->size*2; /* bitmap height */
	int l_h = font->size*2; /* line height */
	
	font->palette_width = b_w;
	font->palette_height = b_h;
	
	/* create a bitmap for the phrase */
    unsigned char* bitmap = malloc(b_w * b_h);
	memset(bitmap, 0, b_w * b_h);
	
	/* calculate font scaling */
	float scale = stbtt_ScaleForPixelHeight(&info, l_h);
	
	int x = 0;
	
    int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
	
	ascent *= scale;
	descent *= scale;
	
    for (int i = 32; i <= 126; ++i)
    {
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
        
		font->glyph_widths[i-32] = (ax*scale);
	}
	
	font->info = info;
	font->scale = scale;
	
	font->bitmap = bitmap;
	free(ttf_buffer);
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
		
		// 1 us
		thread_sleep(1);
	}
	
	return 0;
}

image *assets_load_image(char *file)
{
	// check if image is already loaded or loading
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img_at = array_at(&global_asset_collection.images, i);
		
		if (strcmp(img_at->path, file) == 0)
		{
			// image is already loaded/loading
			img_at->references++;
			return img_at;
		}
	}
	
	image new_image;
	new_image.path = malloc(strlen(file)+1);
	strcpy(new_image.path, file);
	new_image.loaded = false;
	
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
	image *image_at = 0;
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image_at = array_at(&global_asset_collection.images, i);
		
		if (strcmp(image_at->path, image_to_destroy->path) == 0)
		{
			// image is already loaded/loading
			image_at->references--;
			goto image_found;
		}
	}
	return;
	
	image_found:
	if (image_at->references <= 0)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &image_at->textureID);
		
		free(image_at->path);
		array_remove(&global_asset_collection.images, image_at);
	}
}

font *assets_load_font(char *file, s16 size)
{
	assert(!(size % 4));
	for (int i = 0; i < global_asset_collection.fonts.length; i++)
	{
		font *font_at = array_at(&global_asset_collection.fonts, i);
		
		if (strcmp(font_at->path, file) == 0 && font_at->size == size)
		{
			// font is already loaded/loading
			font_at->references++;
			return font_at;
		}
	}
	
	font new_font;
	new_font.path = malloc(strlen(file)+1);
	new_font.size = size;
	strcpy(new_font.path, file);
	new_font.loaded = false;
	
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
	font *font_at = 0;
	for (int i = 0; i < global_asset_collection.fonts.length; i++)
	{
		font_at = array_at(&global_asset_collection.fonts, i);
		
		if (font_at->size == font_to_destroy->size &&
			strcmp(font_at->path, font_to_destroy->path) == 0)
		{
			font_at->references--;
			goto font_found;
		}
	}
	return;
	
	font_found:
	if (font_at->references <= 0)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &font_at->textureID);
		
		free(font_at->path);
		array_remove(&global_asset_collection.fonts, font_at);
	}
}

sample *assets_load_sample(char *file)
{
	for (int i = 0; i < global_asset_collection.samples.length; i++)
	{
		sample *sample_at = array_at(&global_asset_collection.fonts, i);
		
		if (strcmp(sample_at->path, file) == 0)
		{
			// sample is already loaded/loading
			sample_at->references++;
			return sample_at;
		}
	}
	
	sample new_sample;
	new_sample.path = malloc(strlen(file)+1);
	strcpy(new_sample.path, file);
	new_sample.loaded = true;
	new_sample.references = 1;
	
	set_active_directory(binary_path);
	
	unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    s16* pSampleData = drwav_open_file_and_read_pcm_frames_s16(file, 
															   &channels, &sampleRate, 
															   &totalPCMFrameCount);
    if (pSampleData == NULL) {
		new_sample.loaded = false;
		goto failure;
	}
	
	new_sample.sample_count = totalPCMFrameCount;
	new_sample.sample_rate = sampleRate;
	new_sample.channels = channels;
	new_sample.duration_seconds = totalPCMFrameCount / (float)sampleRate;
	new_sample.data = pSampleData;
	
	assert(new_sample.channels == 2);
	// TODO(Aldrik): should we assert sample rate?
	
	// NOTE(Aldrik): we should never realloc the sample array because pointers will be 
	// invalidated.
	assert(global_asset_collection.samples.reserved_length > global_asset_collection.samples.length);
	
	failure:
	{
		int index = array_push(&global_asset_collection.samples, &new_sample);
		
		return array_at(&global_asset_collection.samples, index);
	}
}

void assets_destroy_sample(sample *sample_to_destroy)
{
	sample *sample_at = 0;
	for (int i = 0; i < global_asset_collection.samples.length; i++)
	{
		sample_at = array_at(&global_asset_collection.samples, i);
		
		if (strcmp(sample_at->path, sample_to_destroy->path) == 0)
		{
			sample_at->references--;
			goto sample_found;
		}
	}
	return;
	
	sample_found:
	if (sample_at->references <= 0)
	{
		drwav_free(sample_at->data);
		free(sample_at->path);
		array_remove(&global_asset_collection.samples, sample_at);
	}
}


void assets_destroy()
{
	global_asset_collection.valid = false;
	
	array_destroy(&global_asset_collection.images);
	array_destroy(&global_asset_collection.fonts);
	array_destroy(&global_asset_collection.samples);
	
	array_destroy(&global_asset_collection.queue.queue);
	array_destroy(&global_asset_collection.post_process_queue);
	
	mutex_destroy(&asset_mutex);
}
