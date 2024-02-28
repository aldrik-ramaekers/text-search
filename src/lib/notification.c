/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

void push_notification(char *message)
{
	if (!global_notifications.data)
	{
		global_notifications = array_create(sizeof(notification));
		array_reserve(&global_notifications, 10);
	}
	
	s32 len = strlen(message)+1;
	
	notification new_notification;
	new_notification.message = mem_alloc(len);
	new_notification.duration = 0;
	string_copyn(new_notification.message, message, len);
	array_push(&global_notifications, &new_notification);
}

void update_render_notifications()
{
	const s32 padding = 10;
	const s32 box_h = global_ui_context.font_small->px_h + (padding*2);
	const float32 fade_duration = 0.1f;
	const float32 show_duration = 1.0f;
	
	for (s32 i = 0; i < global_notifications.length; i++)
	{
		main_window->do_draw = true;
		
		notification *n = array_at(&global_notifications, i);
		float32 duration_ms = (float32)n->duration/TARGET_FRAMERATE;
		s32 y = 0;
		
		if (duration_ms < fade_duration)
			y = main_window->height - ((duration_ms/fade_duration)*(box_h + 30.0f));
		else if (duration_ms > show_duration)
			y = main_window->height - ((box_h + 30.0f) - ((duration_ms-show_duration)/fade_duration)*(box_h + 30.0f));
		else 
			y = main_window->height - box_h - 30;
		
		s32 w = calculate_text_width(global_ui_context.font_small, n->message) + (padding*2);
		s32 x = 30;
		
		render_rectangle(x+4, y+4, w,box_h, rgba(100,0,0, 100));
		render_rectangle(x, y, w,box_h, rgb(200,0,0));
		render_text(global_ui_context.font_small, x + padding, y + padding, n->message, rgb(255,255,255));
		
		n->duration++;
		
		if (duration_ms > show_duration+fade_duration)
			array_remove_at(&global_notifications, i);
		break;
	}
}
