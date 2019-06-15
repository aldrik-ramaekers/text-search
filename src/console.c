
void console_create()
{
	global_console.log = array_create(sizeof(console_message));
	console_mutex = mutex_create();
}

void console_update_render(platform_window *window, camera *camera, mouse_input *mouse, s32 x, s32 y, s32 w, s32 h)
{
	render_rectangle(x+camera->x, y+camera->y, w, h, WATCH_WINDOW_BACKGROUND_COLOR);
	render_rectangle_outline(x+camera->x, y+camera->y, w, h, 1, WATCH_WINDOW_BORDER_COLOR);
	render_set_scissor(window, x, y, w, h);
	
	static s32 scroll_y = 0;
	
	if (mouse->scroll_state == SCROLL_UP)
		scroll_y+=WATCH_WINDOW_SCROLL_SPEED;
	if (mouse->scroll_state == SCROLL_DOWN)
		scroll_y-=WATCH_WINDOW_SCROLL_SPEED;
	
	if (scroll_y < 0)
		scroll_y = 0;
	
	y += scroll_y;
	
	mutex_lock(&console_mutex);
	{
		s32 th = 0;
		for (int i = global_console.log.length-1; i >= 0; i--)
		{
			console_message *msg = array_at(&global_console.log, i);
			
			color c = CONSOLE_MESSAGE_COLOR;
			
			if (msg->type == PRINT_ERROR)
				c = CONSOLE_ERROR_COLOR;
			
			render_text_cutoff(global_info_menu.font_small, x + 10+camera->x, y + h - 100 - th +camera->y, msg->message, c, w-30);
			
			th += calculate_text_height(global_info_menu.font_small, w-30, msg->message);
		}
	}
	mutex_unlock(&console_mutex);
	
	render_reset_scissor();
}

void console_print(char *message, console_print_type type, ...)
{
	char tmp[500];
	
	va_list args;
	va_start(args, type);
	vsprintf(tmp, message, args);
	va_end(args);
	
	char *msg = mem_alloc(300);
	
	u64 ms = platform_get_time(TIME_PROCESS, TIME_MILI_S);
	u32 time_ms = ms % 1000;
	u32 time_s = ms / 1000;
	
	sprintf(msg, "[%d.%03d] %s", time_s, time_ms, tmp);
	
	console_message new_message;
	new_message.message = msg;
	new_message.type = type;
	
	mutex_lock(&console_mutex);
	{
		array_push(&global_console.log, &new_message);
	}
	mutex_unlock(&console_mutex);
}

void console_destroy()
{
	for (int i = 0; i < global_console.log.length; i++)
	{
		console_message *m = array_at(&global_console.log, i);
		mem_free(m->message);
	}
	array_destroy(&global_console.log);
	mutex_destroy(&console_mutex);
}
