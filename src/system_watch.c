
void system_watch_update_render(platform_window *window, camera *camera, s32 x, s32 y, s32 w, s32 h)
{
	static bool inited = false;
	if (!inited)
	{
		global_system_watch.memory_size = platform_get_memory_size();
		global_system_watch.cpu_count = platform_get_cpu_count();
		global_system_watch.cpu = platform_get_cpu_info();
		inited = true;
	}
	
	render_rectangle(x+camera->x, y+camera->y, w, h, WATCH_WINDOW_BACKGROUND_COLOR);
	render_rectangle_outline(x+camera->x, y+camera->y, w, h, 1, WATCH_WINDOW_BORDER_COLOR);
	render_set_scissor(window, x, y, w, h);
	
	x += 10;
	y += 10;
	
	{
		char cpu_buffer[90];
		sprintf(cpu_buffer, "%-10s %27d", "cpu count:", global_system_watch.cpu_count);
		
		render_text(global_info_menu.font_small, x+camera->x, y+camera->y, 
					cpu_buffer, TOOLTIP_FOREGROUND_COLOR);
		y += global_info_menu.font_small->size;
	}
	
	{
		char memory_buffer[90];
		sprintf(memory_buffer, "%-10s %25dmb", "memory:", global_system_watch.memory_size);
		
		render_text(global_info_menu.font_small, x+camera->x, y+camera->y, 
					memory_buffer, TOOLTIP_FOREGROUND_COLOR);
		y += global_info_menu.font_small->size;
	}
	
	{
		char model_buffer[90];
		sprintf(model_buffer, "%-10s %27d", "model:", global_system_watch.cpu.model);
		
		render_text(global_info_menu.font_small, x+camera->x, y+camera->y, 
					model_buffer, TOOLTIP_FOREGROUND_COLOR);
		y += global_info_menu.font_small->size;
	}
	
	{
		char frequency_buffer[90];
		sprintf(frequency_buffer, "%-10s %27.4f", "frequency:", global_system_watch.cpu.frequency);
		
		render_text(global_info_menu.font_small, x+camera->x, y+camera->y, 
					frequency_buffer, TOOLTIP_FOREGROUND_COLOR);
		y += global_info_menu.font_small->size;
	}
	
	
	{
		char size_buffer[90];
		sprintf(size_buffer, "%-10s %24dkb", "cache size:", global_system_watch.cpu.cache_size);
		
		render_text(global_info_menu.font_small, x+camera->x, y+camera->y, 
					size_buffer, TOOLTIP_FOREGROUND_COLOR);
		y += global_info_menu.font_small->size;
	}
	
	{
		char alignment_buffer[90];
		sprintf(alignment_buffer, "%-10s %21d", "cache alignment:", global_system_watch.cpu.cache_alignment);
		
		render_text(global_info_menu.font_small, x+camera->x, y+camera->y, 
					alignment_buffer, TOOLTIP_FOREGROUND_COLOR);
		y += global_info_menu.font_small->size;
	}
	
	render_reset_scissor();
}