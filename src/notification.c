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

void notification_manager_init()
{
	global_notification_manager.messages = array_create(MAX_INPUT_LENGTH);
	array_reserve(&global_notification_manager.messages, 10);
	global_notification_manager.current_notification_display_stamp = 0;
}

void show_notification(char message[MAX_INPUT_LENGTH])
{
	array_push(&global_notification_manager.messages, message);
	
	if (global_notification_manager.current_notification_display_stamp == 0)
	{
		global_notification_manager.current_notification_display_stamp = platform_get_time(TIME_FULL, TIME_MILI_S);
	}
	else if (global_notification_manager.messages.length == 1)
	{
		global_notification_manager.current_notification_display_stamp = platform_get_time(TIME_FULL, TIME_MILI_S);
	}
}

void update_render_notifications(platform_window *window)
{
	if (global_notification_manager.messages.length)
	{
		char *message = array_at(&global_notification_manager.messages, 0);
		
		u64 duration = platform_get_time(TIME_FULL, TIME_MILI_S) - global_notification_manager.current_notification_display_stamp;
		
		s32 y = window->height-20;
		s32 x = 30;
		s32 text_h = global_ui_context.font_small->size+20;
		s32 total_w = calculate_text_width(global_ui_context.font_small, message) +
			(WIDGET_PADDING*4);
		
		if (duration < 150)
		{
			y -= duration/4;
		}
		else if (duration > 1250)
		{
			y -= (1450-duration)/4;
		}
		else
		{
			y -= 150/4;
		}
		
		render_rectangle(x, y, total_w, text_h, global_ui_context.style.item_hover_background);
		render_rectangle_outline(x, y, total_w, text_h, 1, global_ui_context.style.border);
		render_text(global_ui_context.font_small, x + WIDGET_PADDING*2, y + 10, message, global_ui_context.style.foreground);
		
		if (duration >= 1500)
		{
			array_remove_at(&global_notification_manager.messages, 0);
			
			if (global_notification_manager.messages.length != 0)
				global_notification_manager.current_notification_display_stamp = platform_get_time(TIME_FULL, TIME_MILI_S);
		}
	}
}

void notification_manager_destroy()
{
	array_destroy(&global_notification_manager.messages);
}
