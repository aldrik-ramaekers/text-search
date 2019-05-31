static inline bool asset_pipeline_watch_push_task(asset_task *task, s32 *x, s32 *y, mouse_input *mouse, s32 index, font *fnt, camera *camera)
{
	bool hovered = (mouse->x >= *x-camera->x && mouse->x <= *x-camera->x+WATCH_WINDOW_WIDTH && mouse->y > *y-camera->y && mouse->y <= *y-camera->y + fnt->size);
	
	char *path = 0;
	switch(task->type)
	{
		case ASSET_IMAGE: path = task->image->path; break;
		case ASSET_FONT: path = task->font->path; break;
	}
	
	char entry_buffer[50];
	sprintf(entry_buffer, "%02d %s", index, path);
	
	if (hovered)
		render_rectangle(*x, *y, WATCH_WINDOW_WIDTH, global_info_menu.font_small->size, WATCH_WINDOW_ENTRY_HOVER_BACKGROUMD_COLOR);
	
	render_text(fnt, *x, *y, entry_buffer, TOOLTIP_FOREGROUND_COLOR);
	
	*y += fnt->size;
	
	return hovered;
}

static inline bool asset_pipeline_watch_push_loaded_asset(s32 references, char *path, s32 *x, s32 *y, font *fnt, s32 *index, mouse_input *mouse, camera *camera)
{
	char entry_buffer[50];
	sprintf(entry_buffer, "%02d [%d] %s", *index, references, path);
	*index = *index+1;
	bool hovered = (mouse->x >= *x-camera->x && mouse->x <= *x-camera->x+WATCH_WINDOW_WIDTH && mouse->y > *y-camera->y && mouse->y <= *y-camera->y + fnt->size);
	
	if (hovered)
		render_rectangle(*x, *y,  WATCH_WINDOW_WIDTH, fnt->size, WATCH_WINDOW_ENTRY_HOVER_BACKGROUMD_COLOR);
	
	render_text(fnt, *x, *y, entry_buffer, TOOLTIP_FOREGROUND_COLOR);
	*y += fnt->size;
	
	return hovered;
}

static inline void asset_pipeline_watch_push_title(font *fnt, char *title, s32 *x, s32 *y)
{
	render_text(fnt, *x, *y, title, TOOLTIP_FOREGROUND_COLOR);
	*y += fnt->size;
}

void asset_pipeline_watch_update_render(platform_window *window, camera *camera, mouse_input *mouse, s32 x, s32 y, s32 w, s32 h)
{
	
	render_rectangle(x+camera->x, y+camera->y, w, h, WATCH_WINDOW_BACKGROUND_COLOR);
	render_rectangle_outline(x+camera->x, y+camera->y, w, h, 1, WATCH_WINDOW_BORDER_COLOR);
	render_set_scissor(window, x, y, w, h);
	
	// 1. draw post process queue
	// 2. draw task queue
	// 3. draw loaded asset list with reference count and load time
	
	font *fnt = global_info_menu.font_small;
	font *fnt_m = global_info_menu.font_medium;
	
	y += 10 + camera->y;
	x += 10 + camera->x;
	
	static s32 scroll_y = 0;
	
	if (mouse->scroll_state == SCROLL_UP)
		scroll_y+=WATCH_WINDOW_SCROLL_SPEED;
	if (mouse->scroll_state == SCROLL_DOWN)
		scroll_y-=WATCH_WINDOW_SCROLL_SPEED;
	if (scroll_y > 0)
		scroll_y = 0;
	y += scroll_y;
	
	mutex_lock(&asset_mutex);
	
	asset_pipeline_watch_push_title(fnt_m, "post process queue", &x, &y);
	for (int i = 0; i < global_asset_collection.post_process_queue.length; i++)
	{
		asset_task *task = array_at(&global_asset_collection.post_process_queue, i);
		bool hovered = asset_pipeline_watch_push_task(task, &x, &y, mouse, i, fnt, camera);
	}
	
	// padding
	y += fnt->size;
	asset_pipeline_watch_push_title(fnt_m, "asset queue", &x, &y);
	
	for (int i = 0; i < global_asset_collection.queue.queue.length; i++)
	{
		asset_task *task = array_at(&global_asset_collection.queue.queue, i);
		bool hovered = asset_pipeline_watch_push_task(task, &x, &y, mouse, i, fnt, camera);
	}
	
	// padding
	y += fnt->size;
	asset_pipeline_watch_push_title(fnt_m, "loaded assets", &x, &y);
	
	int tt = 0;
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img = array_at(&global_asset_collection.images, i);
		bool hovered = asset_pipeline_watch_push_loaded_asset(img->references, img->path, 
															  &x, &y, fnt, &tt, mouse, camera);
		
		if (hovered)
		{
			render_reset_scissor();
			
			s32 r_width = img->width;
			s32 r_height = img->height;
			
			if (r_width > 1000)
			{
				r_width /= 4;
				r_height /= 4;
			}
			else if (r_width > 300)
			{
				r_width /= 2;
				r_height /= 2;
			}
			
			render_rectangle(x - 30 - r_width - 10, 
							 y - 10, r_width + 20, 
							 r_height + 20, TOOLTIP_BACKGROUND_COLOR);
			
			render_image(img, x - 30 - r_width, y, r_width, r_height);
			render_set_scissor(window, x-10-camera->x, y-10-camera->y, w, h);
		}
	}
	
	for (int i = 0; i < global_asset_collection.fonts.length; i++)
	{
		font *c_fnt = array_at(&global_asset_collection.fonts, i);
		bool hovered = asset_pipeline_watch_push_loaded_asset(c_fnt->references, c_fnt->path, 
															  &x, &y, fnt, &tt, mouse, camera);
		
		if (hovered)
		{
			render_reset_scissor();
			
			int text_w = 12 * (c_fnt->size/1.5);
			render_rectangle(x - text_w - 20, y, text_w, c_fnt->size+20, TOOLTIP_BACKGROUND_COLOR);
			render_text(c_fnt, x - text_w -10, y + 10, 
						"hello sailor", TOOLTIP_FOREGROUND_COLOR);
			render_set_scissor(window, x-10-camera->x, y-10-camera->y, w, h);
		}
	}
	
	for (int i = 0; i < global_asset_collection.samples.length; i++)
	{
		sample *sample = array_at(&global_asset_collection.samples, i);
		bool hovered = asset_pipeline_watch_push_loaded_asset(sample->references, sample->path, 
															  &x, &y, fnt, &tt, mouse, camera);
		
		if (hovered)
		{
			render_reset_scissor();
			
			int r_info_h = 30;
			int r_h = 200;
			int r_w = 500;
			
			// graph
			{
				int max_line_height = (r_h - 20) / 4 - 10;
				render_rectangle(x - r_w - 20, y, r_w, r_h+r_info_h, TOOLTIP_BACKGROUND_COLOR);
				
				// 1 sec on screen
				int sec_on_screen = 2;
				int width_per_sec = (r_w-20) / sec_on_screen;
				int jump_width = sample->sample_rate / width_per_sec;
				int max_lines_on_screen = width_per_sec * sec_on_screen;
				
				render_rectangle(x - r_w - 10, y + (r_h/2), r_w-20, 1, rgb(0,0,255));
				
				int k = 0;
				float highest_l = 0;
				float lowest_l = 0;
				float highest_r = 0;
				float lowest_r = 0;
				static int offset = 0;
				//offset += 500;
				for (int s = offset; s < sample->sample_count; s++)
				{
					if (k > max_lines_on_screen) break;
					
					s32 sample_to_display = ((s32*)sample->data)[s];
					
					s16 channel_left = (sample_to_display);
					s16 channel_right = (sample_to_display >> 16);
					
					float percentage_l = channel_left / 32767.0f;
					float line_height_l = percentage_l * max_line_height;
					
					float percentage_r = channel_right / 32767.0f;
					float line_height_r = percentage_r * max_line_height;
					
					if (line_height_l < 1.0f && line_height_l > -1.0f)
						line_height_l = 1.0f;
					if (line_height_r < 1.0f && line_height_r > -1.0f)
						line_height_r = 1.0f;
					
					if (line_height_l < lowest_l)
						lowest_l = line_height_l;
					if (line_height_l > highest_l)
						highest_l = line_height_l;
					
					if (line_height_l < lowest_r)
						lowest_r = line_height_r;
					if (line_height_r > highest_r)
						highest_r = line_height_r;
					
					int oki = s % jump_width;
					if (oki == 0)
					{
						render_rectangle(x - r_w - 10 + k, 
										 y+1 + (r_h/4), 1, -lowest_l, 
										 rgb(255,0,0));
						
						render_rectangle(x - r_w - 10 + k, 
										 y+1 + (r_h/4), 1, -highest_l, 
										 rgb(255,0,0));
						
						render_rectangle(x - r_w - 10 + k, 
										 y+1 + (r_h/2) + max_line_height + 10, 1, -lowest_r, 
										 rgb(255,0,0));
						
						render_rectangle(x - r_w - 10 + k, 
										 y+1 + (r_h/2) + max_line_height + 10, 1, -highest_r, 
										 rgb(255,0,0));
						
						lowest_l = 0;
						highest_l = 0;
						lowest_r = 0;
						highest_r = 0;
						
						k++;
					}
				}
			}
			// info
			{
				char duration_buffer[50];
				sprintf(duration_buffer, "duration: %.2f sec     sample rate: %d", sample->duration_seconds, sample->sample_rate);
				
				render_text(fnt, x-r_w, y + r_h, 
							duration_buffer, TOOLTIP_FOREGROUND_COLOR);
			}
			
			render_set_scissor(window, x-10-camera->x, y-10-camera->y, w, h);
		}
	}
	
	mutex_unlock(&asset_mutex);
	
	render_reset_scissor();
}