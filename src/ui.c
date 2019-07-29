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

inline void ui_begin(s32 id)
{
	global_ui_context.item_hovered = false;
	global_ui_context.next_id = id * 100;
	global_ui_context.layout.offset_x = 0;
	global_ui_context.layout.offset_y = 0;
}

// TODO(Aldrik): LOCALIZE!!
char* name_of_day(s32 day)
{
	switch(day)
	{
		case 0: return "Monday";
		case 1: return "Tuesday";
		case 2: return "Wednesday";
		case 3: return "Thursday";
		case 4: return "Friday";
		case 5: return "Saturday";
		case 6: return "Sunday";
	}
	return 0;
}

char* name_of_month(s32 month)
{
	switch(month)
	{
		case 0: return "Januari";
		case 1: return "Februari";
		case 2: return "March";
		case 3: return "April";
		case 4: return "May";
		case 5: return "June";
		case 6: return "July";
		case 7: return "August";
		case 8: return "September";
		case 9: return "October";
		case 10: return "November";
		case 11: return "December";
	}
	return 0;
}

inline void ui_end()
{
	
}

inline checkbox_state ui_create_checkbox(u8 selected)
{
	checkbox_state state;
	state.state = selected;
	
	return state;
}

inline textbox_state ui_create_textbox(u16 max_len)
{
	assert(max_len > 0);
	assert(max_len <= MAX_INPUT_LENGTH);
	
	textbox_state state;
	state.max_len = max_len;
	state.buffer = mem_alloc(max_len);
	state.buffer[0] = 0;
	state.state = false;
	state.text_offset_x = 0;
	state.history = array_create(sizeof(textbox_history_entry));
	
	return state;
}

void ui_destroy_textbox(textbox_state *state)
{
	for (s32 i = 0; i < state->history.length; i++)
	{
		char **history_entry = array_at(&state->history, i);
		mem_free(*history_entry);
	}
	array_destroy(&state->history);
	
	mem_free(state->buffer);
}

inline button_state ui_create_button()
{
	button_state state;
	state.state = 0;
	
	return state;
}

static s32 get_days_in_month(struct tm *t)
{
	// starting from 1
	s32 month = t->tm_mon + 1;
	s32 year = t->tm_year + 1900;
	
	if (month == 4 || month == 6 || month == 9 || month == 11)
		return 30;
	
	if (month == 2)
	{
		bool leapyear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
		
		if (!leapyear)
			return 28;
		else 
			return 29;
	}
	
	return 31;
}

inline datepicker_state ui_create_datepicker(time_t selected_date)
{
	datepicker_state state;
	state.selected_day = selected_date;
	state.selected_day_info = *localtime(&selected_date);
	
	state.month_first_day = selected_date - ((state.selected_day_info.tm_mday-1)*(60*60*24));
	state.month_first_day_info = *localtime(&state.month_first_day);
	
	state.days_in_current_month = get_days_in_month(&state.month_first_day_info);
	//state.days_in_current_month = 31;
	
	return state;
}

inline scroll_state ui_create_scroll(s32 scroll)
{
	scroll_state state;
	state.scroll = 0;
	state.height = scroll;
	
	return state;
}

inline dropdown_state ui_create_dropdown()
{
	dropdown_state state;
	state.state = 0;
	return state;
}

inline void ui_create(platform_window *window, keyboard_input *keyboard, mouse_input *mouse, camera *camera, font *font_small)
{
	global_ui_context.style.foreground = rgb(250,250,250);
	global_ui_context.style.background = rgb(60,60,60);
	global_ui_context.style.background_hover = rgb(100,100,200);
	global_ui_context.style.border = rgb(40,40,40);
	global_ui_context.style.textbox_background = rgb(100,100,100);
	global_ui_context.style.textbox_active_border = rgb(100,0,0);
	global_ui_context.style.button_background = rgb(100,100,100);
	global_ui_context.style.textbox_foreground = rgb(250,250,250);
	
	global_ui_context.layout.layout_direction = LAYOUT_VERTICAL;
	global_ui_context.layout.offset_x = 0;
	global_ui_context.layout.offset_y = 0;
	global_ui_context.layout.active_window = window;
	global_ui_context.layout.width = global_ui_context.layout.active_window->width;
	global_ui_context.layout.scroll.in_scroll = false;
	
	global_ui_context.keyboard = keyboard;
	global_ui_context.mouse = mouse;
	global_ui_context.font_small = font_small;
	global_ui_context.active_menus = array_create(sizeof(s32));
	global_ui_context.menu_item_count = 0;
	global_ui_context.camera = camera;
	
	array_reserve(&global_ui_context.active_menus, 100);
}

static void ui_pop_scissor()
{
	if (global_ui_context.layout.scroll.in_scroll)
	{
		render_set_scissor(global_ui_context.layout.active_window,
						   global_ui_context.layout.offset_x,
						   global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING + 1, 
						   global_ui_context.layout.width, 
						   global_ui_context.layout.scroll.height + WIDGET_PADDING - 3);
	}
	else
	{
		render_reset_scissor();
	}
}

inline void ui_block_begin(layout_direction direction)
{
	global_ui_context.layout.layout_direction = direction;
	global_ui_context.layout.block_height = 0;
	global_ui_context.layout.start_offset_y = global_ui_context.layout.offset_y;
	global_ui_context.layout.start_offset_x = global_ui_context.layout.offset_x;
	
	ui_pop_scissor();
}

inline void ui_block_end()
{
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
	{
		global_ui_context.layout.offset_y += global_ui_context.layout.block_height + WIDGET_PADDING;
	}
	global_ui_context.layout.offset_x = global_ui_context.layout.start_offset_x;
}

inline void ui_set_active_window(platform_window *window)
{
	global_ui_context.layout.active_window = window;
}

inline void ui_begin_menu_bar()
{
	s32 w = global_ui_context.layout.width;
	s32 h = global_ui_context.layout.active_window->height;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y;
	
	global_ui_context.layout.offset_x = 0;
	global_ui_context.layout.layout_direction = LAYOUT_HORIZONTAL;
	
	render_rectangle(0, y, w, MENU_BAR_HEIGHT, global_ui_context.style.background);
	global_ui_context.layout.menu_offset_y = 0;
}

inline u8 ui_is_menu_active(u32 id)
{
	for (int i = 0; i < global_ui_context.active_menus.length; i++)
	{
		s32 *iid = array_at(&global_ui_context.active_menus, i);
		if (*iid == id) return true;
	}
	return false;
}

inline u32 ui_get_id()
{
	return global_ui_context.next_id++;
}

inline void ui_push_separator()
{
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y;
	s32 w = global_ui_context.layout.width;
	
	render_rectangle(x, y, w, 1, global_ui_context.style.border);
	global_ui_context.layout.offset_y += 1 + WIDGET_PADDING;
}

inline void ui_push_menu_item_separator()
{
	global_ui_context.layout.menu_offset_y += 1;
}

static s32 ui_get_scroll()
{
	if (global_ui_context.layout.scroll.in_scroll)
	{
		return global_ui_context.layout.scroll.scroll;
	}
	
	return 0;
}

u8 ui_push_dropdown_item(image *icon, char *title)
{
	u8 result = false;
	
	u32 id = ui_get_id();
	global_ui_context.layout.dropdown_item_count++;
	s32 h = BUTTON_HEIGHT;
	s32 x = global_ui_context.layout.dropdown_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() + ((global_ui_context.layout.dropdown_item_count)*h-(1*global_ui_context.layout.dropdown_item_count));
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->size/2) + 2;
	s32 total_w = 200 + BUTTON_HORIZONTAL_TEXT_PADDING + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	color bg_color = global_ui_context.style.button_background;
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y > y && mouse_y < y + h)
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		if (is_left_clicked(global_ui_context.mouse))
		{
			result = true;
		}
		
		bg_color = global_ui_context.style.background_hover;
	}
	
	
	render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
	render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, global_ui_context.style.border);
	render_image(icon, x+(BUTTON_HORIZONTAL_TEXT_PADDING/2), y + (h - (h-10))/2, h-10, h-10);
	render_text(global_ui_context.font_small, text_x+(BUTTON_HORIZONTAL_TEXT_PADDING/2)+h-15, text_y, title, global_ui_context.style.foreground);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

u8 ui_push_dropdown(dropdown_state *state, char *title)
{
	u8 result = false;
	
	u32 id = ui_get_id();
	global_ui_context.layout.dropdown_item_count = 0;
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->size/2) + 2;
	s32 total_w = 200 + BUTTON_HORIZONTAL_TEXT_PADDING + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	color bg_color = global_ui_context.style.button_background;
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= y && mouse_y < y + h)
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		if (is_left_clicked(global_ui_context.mouse))
		{
			state->state = !state->state;
		}
		
		bg_color = global_ui_context.style.background_hover;
	}
	else if (is_left_down(global_ui_context.mouse) && state->state)
	{
		state->state = false;
		// render dropdown this frame so item can be selected
		result = true;
	}
	
	render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
	render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, global_ui_context.style.border);
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	
	render_triangle(x+total_w - h, y+(h-(h-12))/2, h-12, h-12, global_ui_context.style.border);
	global_ui_context.layout.dropdown_x = global_ui_context.layout.offset_x;
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result || state->state;
}

u8 ui_push_menu(char *title)
{
	u8 result = false;
	
	global_ui_context.layout.menu_offset_y = 0;
	global_ui_context.menu_item_count = 0;
	u32 id = ui_get_id();
	
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 w = calculate_text_width(global_ui_context.font_small, title) +
		(MENU_HORIZONTAL_PADDING*2);
	s32 text_h = global_ui_context.font_small->size;
	s32 h = MENU_BAR_HEIGHT;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y;
	s32 text_y = global_ui_context.layout.offset_y - (text_h / 2) + (h / 2) + global_ui_context.camera->y;
	s32 text_x = x + MENU_HORIZONTAL_PADDING;
	
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	color bg_color = global_ui_context.style.background;
	
	u8 is_open = ui_is_menu_active(id);
	result = is_open;
	
	if (mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h)
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		if (is_left_clicked(global_ui_context.mouse))
		{
			if (is_open)
				array_remove_by(&global_ui_context.active_menus, &id);
			else
				array_push(&global_ui_context.active_menus, &id);
			
			result = !is_open;
			is_open = result;
		}
		
		bg_color = global_ui_context.style.background_hover;
	}
	else if (is_left_down(global_ui_context.mouse))
	{
		if (is_open)
			array_remove_by(&global_ui_context.active_menus, &id);
		is_open = false;
	}
	
	render_rectangle(x, y, w, h, bg_color);
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	
	global_ui_context.layout.prev_offset_x = global_ui_context.layout.offset_x;
	global_ui_context.layout.offset_x += w;
	
	return result;
}

u8 ui_push_textbox(textbox_state *state, char *placeholder)
{
	u8 result = false;
	static u64 cursor_tick = 0;
	static u64 last_cursor_pos = -1;
	
	if (!global_ui_context.layout.active_window->has_focus)
	{
		state->state = false;
	}
	
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + 5;
	s32 text_y = y + (TEXTBOX_HEIGHT/2) - (global_ui_context.font_small->size/2)+2;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < TEXTBOX_HEIGHT)
		global_ui_context.layout.block_height = TEXTBOX_HEIGHT;
	
	u8 has_text = state->buffer[0] != 0;
	
	if (!state->state)
	{
		render_rectangle(x, y, TEXTBOX_WIDTH, TEXTBOX_HEIGHT, global_ui_context.style.textbox_background);
		render_rectangle_outline(x, y, TEXTBOX_WIDTH, TEXTBOX_HEIGHT, 1, global_ui_context.style.border);
	}
	else
	{
		cursor_tick++;
		render_rectangle(x, y, TEXTBOX_WIDTH, TEXTBOX_HEIGHT, global_ui_context.style.textbox_background);
		render_rectangle_outline(x, y, TEXTBOX_WIDTH, TEXTBOX_HEIGHT, 1, global_ui_context.style.textbox_active_border);
	}
	
	s32 virt_top = y;
	s32 virt_bottom = y + TEXTBOX_HEIGHT;
	if (global_ui_context.layout.scroll.in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll.scroll_start_offset_y + global_ui_context.layout.scroll.height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	bool is_selecting = false;
	bool clicked_to_select = false;
	if (mouse_x >= x && mouse_x < x + TEXTBOX_WIDTH && mouse_y >= virt_top && mouse_y < virt_bottom)
	{
		if (is_left_double_clicked(global_ui_context.mouse) && has_text)
		{
			global_ui_context.keyboard->selection_begin_offset = 0;
			global_ui_context.keyboard->selection_length = strlen(global_ui_context.keyboard->input_text);
			global_ui_context.keyboard->has_selection = true;
			state->selection_start_index = 0;
			
			global_ui_context.mouse->left_state &= ~MOUSE_DOUBLE_CLICK;
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
		}
		if (is_left_clicked(global_ui_context.mouse))
		{
			keyboard_set_input_text(global_ui_context.keyboard, state->buffer);
			cursor_tick = 0;
			
			if (global_ui_context.keyboard->has_selection)
			{
				global_ui_context.keyboard->has_selection = false;
			}
			else if (state->state)
			{
				clicked_to_select = true;
			}
			
			state->state = true;
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			result = true;
			
			global_ui_context.keyboard->take_input = state->state;
		}
		if (is_left_down(global_ui_context.mouse))
		{
			is_selecting = true;
		}
	}
	else if (is_left_clicked(global_ui_context.mouse) || is_left_down(global_ui_context.mouse))
	{
		if (state->state)
		{
			global_ui_context.keyboard->has_selection = false;
		}
		
		state->state = false;
	}
	
	if (keyboard_is_key_pressed(global_ui_context.keyboard, KEY_ENTER))
	{
		global_ui_context.keyboard->has_selection = false;
		state->state = false;
	}
	
	if (global_ui_context.layout.scroll.in_scroll)
	{
		vec4 v = render_get_scissor();
		s32 scissor_x = v.x + WIDGET_PADDING + 5;
		s32 scissor_y = global_ui_context.layout.scroll.scroll_start_offset_y;
		s32 scissor_w = v.w;
		s32 scissor_h = global_ui_context.layout.scroll.height - 2;
		
		render_set_scissor(global_ui_context.layout.active_window, scissor_x,
						   scissor_y, scissor_w, scissor_h);
	}
	else
	{
		s32 scissor_x = x - global_ui_context.camera->x + 5;
		s32 scissor_y = y - global_ui_context.camera->y;
		s32 scissor_w = TEXTBOX_WIDTH - 6;
		s32 scissor_h = TEXTBOX_HEIGHT;
		
		render_set_scissor(global_ui_context.layout.active_window, 
						   scissor_x, scissor_y, scissor_w, scissor_h);
	}
	
	s32 cursor_text_w;
	s32 cursor_x;
	s32 diff = 0;
	
	if (state->state)
	{
		s32 len = strlen(global_ui_context.keyboard->input_text);
		s32 old_len = strlen(state->buffer);
		
		// check if text changes, add to history if true
		u8 is_lctrl_down = global_ui_context.keyboard->keys[KEY_LEFT_CONTROL];
		
		// go to previous state
		if (is_lctrl_down && keyboard_is_key_pressed(global_ui_context.keyboard, KEY_Z) && state->history.length)
		{
			textbox_history_entry *old_text = array_at(&state->history, state->history.length-1);
			strncpy(state->buffer, old_text->text, MAX_INPUT_LENGTH);
			keyboard_set_input_text(global_ui_context.keyboard, state->buffer);
			mem_free(old_text->text);
			array_remove_at(&state->history, state->history.length-1);
			global_ui_context.keyboard->cursor = old_text->cursor_offset;
		}
		else
		{
			if (old_len != len || (last_cursor_pos != global_ui_context.keyboard->cursor && last_cursor_pos != -1))
			{
				textbox_history_entry history_entry;
				history_entry.text = mem_alloc(old_len+1);
				history_entry.cursor_offset = last_cursor_pos;
				strcpy(history_entry.text, state->buffer);
				array_push(&state->history, &history_entry);
			}
			
			strncpy(state->buffer, global_ui_context.keyboard->input_text, state->max_len);
			if (global_ui_context.keyboard->cursor > state->max_len)
			{
				global_ui_context.keyboard->cursor = state->max_len;
				global_ui_context.keyboard->input_text[global_ui_context.keyboard->cursor] = 0;
			}
		}
		
		// cursor ticking after text change
		if (last_cursor_pos != global_ui_context.keyboard->cursor)
			cursor_tick = 0;
		last_cursor_pos = global_ui_context.keyboard->cursor;
		
		// draw cursor
		char calculate_text[MAX_INPUT_LENGTH];
		strcpy(calculate_text, state->buffer);
		calculate_text[global_ui_context.keyboard->cursor] = 0;
		
		cursor_text_w = calculate_text_width(global_ui_context.font_small, 
											 calculate_text);
		
		cursor_x = text_x + cursor_text_w;
		
		if (cursor_x > text_x + TEXTBOX_WIDTH-10)
		{
			cursor_x = text_x + TEXTBOX_WIDTH-10;
		}
		
		if (cursor_text_w > TEXTBOX_WIDTH - 10)
		{
			diff = cursor_text_w - TEXTBOX_WIDTH + 10;
		}
		
		s32 cursor_y = text_y - 2;
		s32 cursor_h = global_ui_context.font_small->size + 1;
		s32 cursor_w = 2;
		
		if (cursor_tick % 50 < 25 && !global_ui_context.keyboard->has_selection)
			render_rectangle(cursor_x, cursor_y, cursor_w, cursor_h, global_ui_context.style.textbox_foreground);
	}
	if (clicked_to_select)
	{
		global_ui_context.keyboard->has_selection = true;
		global_ui_context.keyboard->selection_begin_offset = calculate_cursor_position(global_ui_context.font_small, 
																					   state->buffer, mouse_x + diff - text_x);
		global_ui_context.keyboard->selection_length = 1;
		state->selection_start_index = global_ui_context.keyboard->selection_begin_offset;
	}
	if (is_selecting)
	{
		s32 index = calculate_cursor_position(global_ui_context.font_small, 
											  state->buffer, mouse_x + diff - text_x)+1;
		
		s32 right = global_ui_context.keyboard->selection_begin_offset + global_ui_context.keyboard->selection_length;
		s32 left = global_ui_context.keyboard->selection_begin_offset;
		
		if (index < state->selection_start_index)
		{
			global_ui_context.keyboard->selection_begin_offset = index - 1;
			global_ui_context.keyboard->selection_length = state->selection_start_index - index + 2;
		}
		else if (index > state->selection_start_index)
		{
			global_ui_context.keyboard->selection_begin_offset = state->selection_start_index;
			global_ui_context.keyboard->selection_length = index - state->selection_start_index;
		}
	}
	if (global_ui_context.keyboard->has_selection && state->state)
	{
		strncpy(state->buffer, global_ui_context.keyboard->input_text, state->max_len);
		
		// calculate selection start x
		char ch = state->buffer[global_ui_context.keyboard->selection_begin_offset];
		state->buffer[global_ui_context.keyboard->selection_begin_offset] = 0;
		s32 selection_start_x =  calculate_text_width(global_ui_context.font_small, state->buffer);
		state->buffer[global_ui_context.keyboard->selection_begin_offset] = ch;
		
		// calculate selection width
		ch = state->buffer[global_ui_context.keyboard->selection_begin_offset+global_ui_context.keyboard->selection_length];
		state->buffer[global_ui_context.keyboard->selection_begin_offset+global_ui_context.keyboard->selection_length] = 0;
		s32 selection_width =  calculate_text_width(global_ui_context.font_small, state->buffer+global_ui_context.keyboard->selection_begin_offset);
		state->buffer[global_ui_context.keyboard->selection_begin_offset+global_ui_context.keyboard->selection_length] = ch;
		
		render_rectangle(text_x - diff + selection_start_x, y+4, selection_width, TEXTBOX_HEIGHT-8, global_ui_context.style.textbox_active_border);
	}
	if (!has_text)
	{
		render_text(global_ui_context.font_small, text_x - diff, text_y, 
					placeholder, global_ui_context.style.textbox_foreground);
	}
	else
	{
		render_text(global_ui_context.font_small, text_x - diff, text_y, 
					state->buffer, global_ui_context.style.foreground);
	}
	
	ui_pop_scissor();
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += TEXTBOX_WIDTH + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += TEXTBOX_HEIGHT + WIDGET_PADDING;
	
	return result;
}

void ui_push_text(char *text)
{
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + WIDGET_PADDING;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (global_ui_context.font_small->size/2) + spacing_y + 2;
	s32 total_w = calculate_text_width(global_ui_context.font_small, text) +
		WIDGET_PADDING + WIDGET_PADDING;
	
	if (global_ui_context.layout.block_height < global_ui_context.font_small->size)
		global_ui_context.layout.block_height = global_ui_context.font_small->size;
	
	render_text(global_ui_context.font_small, text_x, text_y, text, global_ui_context.style.foreground);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += CHECKBOX_SIZE + WIDGET_PADDING;
}

u8 ui_push_checkbox(checkbox_state *state, char *title)
{
	u8 result = false;
	
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + CHECKBOX_SIZE + WIDGET_PADDING;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (global_ui_context.font_small->size/2) + spacing_y + 2;
	s32 total_w = calculate_text_width(global_ui_context.font_small, title) +
		CHECKBOX_SIZE + WIDGET_PADDING + WIDGET_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < CHECKBOX_SIZE)
		global_ui_context.layout.block_height = CHECKBOX_SIZE;
	
	render_rectangle_outline(x, y, CHECKBOX_SIZE, CHECKBOX_SIZE, 1, global_ui_context.style.border);
	
	s32 virt_top = y;
	s32 virt_bottom = y + CHECKBOX_SIZE;
	if (global_ui_context.layout.scroll.in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll.scroll_start_offset_y + global_ui_context.layout.scroll.height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + CHECKBOX_SIZE && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		if (is_left_clicked(global_ui_context.mouse))
		{
			state->state = !state->state;
			result = true;
		}
	}
	
	if (state->state)
	{
		s32 spacing = 2;
		render_rectangle(x+spacing, y+spacing, CHECKBOX_SIZE-(spacing*2), CHECKBOX_SIZE-(spacing*2), global_ui_context.style.border);
	}
	
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += CHECKBOX_SIZE + WIDGET_PADDING;
	
	return result;
}

inline u8 is_shortcut_down(s32 shortcut_keys[2])
{
	return keyboard_is_key_down(global_ui_context.keyboard, shortcut_keys[0]) &&
		keyboard_is_key_pressed(global_ui_context.keyboard, shortcut_keys[1]);
}

u8 ui_push_menu_item(char *title, char *shortcut)
{
	u8 result = false;
	
	set_render_depth(30);
	
	global_ui_context.menu_item_count++;
	
	s32 x = global_ui_context.layout.prev_offset_x + global_ui_context.camera->x;
	s32 w = MENU_ITEM_WIDTH;
	s32 text_h = global_ui_context.font_small->size;
	s32 h = MENU_BAR_HEIGHT;
	s32 y = global_ui_context.layout.offset_y + (global_ui_context.menu_item_count * h)+1 +
		global_ui_context.layout.menu_offset_y + global_ui_context.camera->y;
	s32 text_y = y - (text_h / 2) + (h / 2);
	s32 text_x = x + MENU_HORIZONTAL_PADDING;
	s32 text_2_x = x + w - MENU_HORIZONTAL_PADDING
		- calculate_text_width(global_ui_context.font_small, shortcut);
	
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	color bg_color = global_ui_context.style.background;
	
	if ((mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h))
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		bg_color = global_ui_context.style.background_hover;
		global_ui_context.item_hovered = true;
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			result = true;
		}
	}
	
	render_rectangle(x, y, w, h, bg_color);
	render_rectangle(x, y+MENU_BAR_HEIGHT, w, 1, global_ui_context.style.border);
	
	// borders
	render_rectangle(x, y, w, 1,  bg_color);
	render_rectangle(x, y, 1, MENU_BAR_HEIGHT, global_ui_context.style.border);
	render_rectangle(x+w, y, 1, MENU_BAR_HEIGHT+1, global_ui_context.style.border);
	
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	render_text(global_ui_context.font_small, text_2_x, text_y, shortcut, global_ui_context.style.foreground);
	
	set_render_depth(1);
	
	return result;
}

u8 ui_push_datepicker(datepicker_state *state, s32 tile_w)
{
	u8 result = false;
	
	s32 top_bar_h = 60;
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 total_w = tile_w * 7;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = (tile_w * 6) + top_bar_h;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll.in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll.scroll_start_offset_y + global_ui_context.layout.scroll.height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	render_rectangle_outline(x,y,total_w,top_bar_h,1,global_ui_context.style.border);
	//render_rectangle_outline(x,y+top_bar_h,total_w,h-top_bar_h,1,global_ui_context.style.border);
	
	
	// render month name
	{
		char name[50];
		sprintf(name, "%s %d", name_of_month(state->month_first_day_info.tm_mon), 1900+state->month_first_day_info.tm_year);
		s32 posx = x + (total_w/2) - 
			(calculate_text_width(global_ui_context.font_small, name)/2);
		s32 posy = y+10;
		
		render_text(global_ui_context.font_small, posx, posy, name, rgb(30,30,30));
	}
	
	
	// nav buttons
	{
		s32 posx = x+1;
		s32 posy = y+1;
		s32 btnw = 29;
		s32 btnh = top_bar_h-25;
		
		color bg_left = rgb(210,210,210);
		
		if (mouse_x > posx && mouse_x < posx + btnw && mouse_y > posy && mouse_y < posy+btnh && !global_ui_context.item_hovered)
		{
			bg_left = rgb(120,120,120);
			platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
			
			if (is_left_clicked(global_ui_context.mouse))
			{
				*state = ui_create_datepicker(state->month_first_day - (60*60*24));
			}
		}
		
		render_rectangle(posx, posy, btnw, btnh, bg_left);
		
		posx = x + total_w - btnw-1;
		bg_left = rgb(210,210,210);
		
		if (mouse_x > posx && mouse_x < posx + btnw && mouse_y > posy && mouse_y < posy+btnh && !global_ui_context.item_hovered)
		{
			bg_left = rgb(120,120,120);
			platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
			
			if (is_left_clicked(global_ui_context.mouse))
			{
				*state = ui_create_datepicker(state->month_first_day + 
											  (state->days_in_current_month*60*60*24));
			}
		}
		
		
		render_rectangle(posx, posy, btnw, btnh, bg_left);
	}
	
	y += top_bar_h;
	
	// monday -> sunday first letter
	render_rectangle(x, y-24, total_w, 23, global_ui_context.style.border);
	for (s32 i = 0; i < 7; i++)
	{
		s32 posx = x + tile_w * i + 4;
		s32 posy = y - 20;
		
		char name[30];
		strcpy(name, name_of_day(i));
		name[2] = 0;
		
		render_text(global_ui_context.font_small, posx, posy, name, rgb(30,30,30));
	}
	
	// 0 = monday 6 = sunday
	s32 start_day_of_week = state->month_first_day_info.tm_wday-1;
	if (start_day_of_week == -1) start_day_of_week = 6;
	
	s32 current_x = start_day_of_week * tile_w;
	s32 current_y = 0;
	for (s32 i = 0; i < state->days_in_current_month; i++)
	{
		s32 dow = (start_day_of_week + i)%7;
		current_x = dow * tile_w;
		s32 row = (start_day_of_week + i) / 7;
		current_y = row * tile_w;
		
		s32 extra_left = 0;
		if (dow != 0) extra_left = 1;
		
		s32 rec_x = x+current_x-extra_left;
		s32 rec_y = y+current_y-1;
		s32 rec_w = tile_w+extra_left;
		s32 rec_h = tile_w+1;
		
		render_rectangle_outline(rec_x,rec_y,rec_w,rec_h,1,global_ui_context.style.border);
		
		if (mouse_x > rec_x && mouse_x < rec_x + rec_w && mouse_y > rec_y && mouse_y < rec_y+rec_h && !global_ui_context.item_hovered)
		{
			platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
			
			if (is_left_clicked(global_ui_context.mouse))
			{
				state->selected_day = state->month_first_day + (i * (60*60*24));
				state->selected_day_info = *localtime(&state->selected_day);
			}
		}
		
		bool is_selected_day = state->month_first_day_info.tm_yday+i == state->selected_day_info.tm_yday;
		
		if (is_selected_day)
		{
			render_rectangle(rec_x+1,rec_y+1,rec_w-1,rec_h-1,rgb(210,210,210));
		}
		
		
		char day[3];
		sprintf(day, "%d", i+1);
		
		render_text(global_ui_context.font_small, x+current_x+4,y+current_y+4, day, rgb(120,120,120));
	}
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

u8 ui_push_image(image *img, s32 w, s32 h, s32 outline, color tint)
{
	u8 result = false;
	
	if (!img->loaded) return result;
	
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 total_w = w;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll.in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll.scroll_start_offset_y + global_ui_context.layout.scroll.height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
		}
		if (is_left_released(global_ui_context.mouse))
		{
			result = true;
		}
	}
	
	
	render_image_tint(img,x,y,w,h,rgb(200,200,200));
	render_image_tint(img,x+outline,y+outline,w-(outline*2),h-(outline*2),tint);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

u8 ui_push_button(button_state *state, char *title)
{
	u8 result = false;
	
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->size/2) + 2;
	s32 total_w = calculate_text_width(global_ui_context.font_small, title) +
		BUTTON_HORIZONTAL_TEXT_PADDING + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	color bg_color = global_ui_context.style.button_background;
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll.in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll.scroll_start_offset_y + global_ui_context.layout.scroll.height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		bg_color = global_ui_context.style.background_hover;
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			state->state = 1;
		}
		if (is_left_released(global_ui_context.mouse) && state->state)
		{
			state->state = 0;
			result = true;
		}
	}
	if (is_left_released(global_ui_context.mouse))
	{
		//state->state = 0;
	}
	
	render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
	render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, global_ui_context.style.border);
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}


u8 ui_push_button_image(button_state *state, char *title, image *img)
{
	u8 result = false;
	
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->size/2) + 2;
	s32 text_w = calculate_text_width(global_ui_context.font_small, title);
	s32 total_w = text_w + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	
	if (title[0] == 0)
	{
		total_w = 0;
	}
	
	color bg_color = global_ui_context.style.button_background;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	int icon_w = 1;
	int icon_h = 1;
	if (img->loaded)
	{
		float max_icon_size = BUTTON_HEIGHT - (BUTTON_IMAGE_PADDING*2);
		float scale = 1.0f;
		if (img->width >= img->height)
		{
			scale = img->width / max_icon_size;
		}
		else if (img->width > img->height)
		{
			scale = img->height / max_icon_size;
		}
		
		icon_w = img->width / scale;
		icon_h = img->height / scale;
		
		total_w += icon_w + (BUTTON_IMAGE_SPACING*2);
	}
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll.in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll.scroll_start_offset_y + global_ui_context.layout.scroll.height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		platform_set_cursor(global_ui_context.layout.active_window, CURSOR_POINTER);
		bg_color = global_ui_context.style.background_hover;
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			state->state = 1;
		}
		if (is_left_released(global_ui_context.mouse) && state->state)
		{
			state->state = 0;
			result = true;
		}
	}
	if (is_left_released(global_ui_context.mouse))
	{
		state->state = 0;
	}
	
	render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
	render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, global_ui_context.style.border);
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	render_image(img, x + total_w - icon_w - BUTTON_IMAGE_SPACING, y + BUTTON_IMAGE_PADDING, icon_w, icon_h);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

inline void ui_end_menu_bar()
{
	global_ui_context.layout.layout_direction = LAYOUT_VERTICAL;
	global_ui_context.layout.offset_x = 0;
	global_ui_context.layout.offset_y += MENU_BAR_HEIGHT;
}

inline void ui_destroy()
{
	array_destroy(&global_ui_context.active_menus);
}

void ui_scroll_begin(s32 height)
{
	global_ui_context.layout.scroll.in_scroll = true;
	global_ui_context.layout.scroll.height = height;
	
	s32 w = global_ui_context.layout.width - WIDGET_PADDING*2;
	s32 h = height + WIDGET_PADDING;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x + WIDGET_PADDING;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y;
	
	global_ui_context.layout.offset_x += WIDGET_PADDING;
	global_ui_context.layout.offset_y += WIDGET_PADDING;
	global_ui_context.layout.scroll.scroll_start_offset_y = global_ui_context.layout.offset_y;
	
	render_rectangle_outline(x, y, w, h, 1, global_ui_context.style.border);
	render_set_scissor(global_ui_context.layout.active_window, x, y, w, h-2);
}

void ui_scroll_end()
{
	s32 max_scroll = (global_ui_context.layout.scroll.scroll_start_offset_y -
					  global_ui_context.layout.offset_y) + global_ui_context.layout.scroll.height;
	
	global_ui_context.layout.offset_x -= WIDGET_PADDING;
	global_ui_context.layout.offset_y = global_ui_context.layout.scroll.scroll_start_offset_y + global_ui_context.layout.scroll.height;
	global_ui_context.layout.offset_y += WIDGET_PADDING;
	
	// draw scrollbar
	if (max_scroll < 0)
	{
		s32 scroll_y = 0;
		if (global_ui_context.mouse->scroll_state == SCROLL_UP)
			scroll_y+=WATCH_WINDOW_SCROLL_SPEED;
		if (global_ui_context.mouse->scroll_state == SCROLL_DOWN)
			scroll_y-=WATCH_WINDOW_SCROLL_SPEED;
		global_ui_context.layout.scroll.scroll += scroll_y;
		if (global_ui_context.layout.scroll.scroll > 0)
			global_ui_context.layout.scroll.scroll = 0;
		if (global_ui_context.layout.scroll.scroll < max_scroll)
			global_ui_context.layout.scroll.scroll = max_scroll;
		
		float percentage = global_ui_context.layout.scroll.scroll / 
			(float)max_scroll;
		
		float scrollbar_height_percentage = -(max_scroll - global_ui_context.layout.scroll.height) / (float)global_ui_context.layout.scroll.height;
		
		s32 scrollbar_height = global_ui_context.layout.scroll.height / scrollbar_height_percentage;
		s32 scrollbar_pos_y = global_ui_context.layout.scroll.scroll_start_offset_y - WIDGET_PADDING + 1;
		
		s32 tw = global_ui_context.layout.width - WIDGET_PADDING*2;
		s32 tx = global_ui_context.layout.offset_x + global_ui_context.camera->x + WIDGET_PADDING;
		
		s32 scrollbar_pos_x = tx + tw - 11;
		
		scrollbar_pos_y += (global_ui_context.layout.scroll.height - 
							scrollbar_height + WIDGET_PADDING - 2) * percentage;
		
		render_reset_scissor();
		render_rectangle(scrollbar_pos_x, scrollbar_pos_y, 10, scrollbar_height, rgb(200, 0, 0));
	}
	
	global_ui_context.layout.scroll.in_scroll = false;
}
