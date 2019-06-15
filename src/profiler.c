inline void profiler_create()
{
	profiler_mutex = mutex_create();
	global_profiler.results = array_create(sizeof(profiler_result));
	array_reserve(&global_profiler.results, 50);
}

inline u64 profiler_begin_d()
{
	return platform_get_time(TIME_FULL, TIME_NS);
}

inline void profiler_end_d(u64 begin_stamp, const char *function_name, const char *file_name, s32 line_nr)
{
	u64 elapsed = profiler_begin_d() - begin_stamp;
	
	mutex_lock(&profiler_mutex);
	
	for (s32 i = 0; i < global_profiler.results.length; i++)
	{
		profiler_result *result = array_at(&global_profiler.results, i);
		
		if (strcmp(result->name, function_name) == 0)
		{
			result->sample_count++;
			result->total_ns += elapsed;
			goto profiling_finished;
		}
	}
	
	profiler_result new_result;
	new_result.sample_count = 1;
	new_result.total_ns = elapsed;
	
	new_result.name = mem_alloc(255);
	new_result.file = mem_alloc(255);
	strcpy(new_result.name, function_name);
	strcpy(new_result.file, file_name);
	new_result.line_nr = line_nr;
	array_push(&global_profiler.results, &new_result);
	
	profiling_finished:
	mutex_unlock(&profiler_mutex);
}

inline void profiler_destroy()
{
	for (int i = 0; i < global_profiler.results.length; i++)
	{
		profiler_result *result = array_at(&global_profiler.results, i);
		mem_free(result->name);
		mem_free(result->file);
	}
	
	array_destroy(&global_profiler.results);
	mutex_destroy(&profiler_mutex);
}

inline static u8  profiler_push_entry(profiler_result *result, s32 x, s32 y, s32 tick, float64 avg, float64 highest_avg, mouse_input *mouse, camera *camera)
{
	char text_buffer[500];
	char identifier_buffer[300];
	char ns_buffer[100];
	
	u8 hovered = (mouse->x >= x-camera->x && mouse->x <= x-camera->x+WATCH_WINDOW_WIDTH && mouse->y > y-camera->y && mouse->y <= y-camera->y + global_info_menu.font_small->size);
	
	if (avg < 1000)
	{
		sprintf(ns_buffer, "%.0fns", avg);
	}
	else if (avg < 1000000)
	{
		sprintf(ns_buffer, "%.2fus", avg/1000);
	} 
	else
	{
		sprintf(ns_buffer, "%.2fms", avg/1000000);
	}
	
	int call_count = result->sample_count / tick;
	
	sprintf(identifier_buffer, "%4d %-16.16s", call_count, result->name);
	
	sprintf(text_buffer, "%s : %-8s %6.2f%%", 
			identifier_buffer, 
			ns_buffer, (avg/highest_avg)*100);
	
	if (hovered)
		render_rectangle(x, y, WATCH_WINDOW_WIDTH, global_info_menu.font_small->size, WATCH_WINDOW_ENTRY_HOVER_BACKGROUMD_COLOR);
	
	render_text(global_info_menu.font_small, x, y, 
				text_buffer, TOOLTIP_FOREGROUND_COLOR);
	
	return hovered;
}

void profiler_update_render(platform_window *window, s32 x, s32 y, s32 w, s32 h, camera *camera, u8 render, mouse_input *mouse)
{
	mutex_lock(&profiler_mutex);
	
	float64 last_avg = 0.0;
	static float64 highest_avg = 1.0;
	
	if(render)
	{
		render_rectangle(x+camera->x, y+camera->y, w, h, WATCH_WINDOW_BACKGROUND_COLOR);
		render_rectangle_outline(x+camera->x, y+camera->y, w, h, 1, WATCH_WINDOW_BORDER_COLOR);
		render_set_scissor(window, x, y, w, h);
	}
	
	x += 10;
	y += 10;
	
	static s32 tick = 0;
	static s32 scroll_y = 0;
	
	if (render)
	{
		if (mouse->scroll_state == SCROLL_UP)
			scroll_y+=WATCH_WINDOW_SCROLL_SPEED;
		if (mouse->scroll_state == SCROLL_DOWN)
			scroll_y-=WATCH_WINDOW_SCROLL_SPEED;
		if (scroll_y > 0)
			scroll_y = 0;
		y += scroll_y;
	}
	tick++;
	
	for (s32 i = 0; i < global_profiler.results.length; i++)
	{
		profiler_result *result = array_at(&global_profiler.results, i);
		
		float64 avg = result->total_ns / tick;
		
		if (render)
		{
			u8 hovered = profiler_push_entry(result, x+camera->x, y+camera->y, 
											 tick, avg, highest_avg, mouse, camera);
			
			if (hovered)
			{
				char file_buffer[520];
				sprintf(file_buffer, "%s:%d [%s]", result->file, result->line_nr, result->name);
				
				int h_w = (global_info_menu.font_small->size * strlen(file_buffer))/1.5f;
				int h_x = x - h_w - 20+camera->x;
				int h_y = y+camera->y;
				int h_h = global_info_menu.font_small->size+20;
				
				render_reset_scissor();
				render_rectangle(h_x, h_y, h_w, h_h, TOOLTIP_BACKGROUND_COLOR);
				render_text(global_info_menu.font_small, h_x+10, h_y+10, file_buffer,  TOOLTIP_FOREGROUND_COLOR);
				render_set_scissor(window, x, y, w, h);
			}
			
			y += global_info_menu.font_small->size;
		}
		
		
		if (tick >= 60)
		{
			result->total_ns = 0;
			result->sample_count = 0;
		}
		else if (result->sample_count == 0)
		{
			array_remove_at(&global_profiler.results, i);
			--i;
			continue;
		}
		
		if (i && avg > last_avg)
		{
			array_swap(&global_profiler.results, i, i-1);
		}
		
		if (i == 0)
			highest_avg = avg;
		
		last_avg = avg;
	}
	
	if (render)
		render_reset_scissor();
	
	if (tick >= 60)
	{
		tick = 0;
	}
	
	mutex_unlock(&profiler_mutex);
}