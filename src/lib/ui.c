/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

void ui_set_hovered(u32 id, s32 x, s32 y, s32 w, s32 h)
{
	global_ui_context.item_hovered = true;
	global_ui_context.tooltip.x = x;
	global_ui_context.tooltip.y = y;
	global_ui_context.tooltip.w = w;
	global_ui_context.tooltip.h = h;
	
	if (global_ui_context.item_hovered_id == id)
		global_ui_context.item_hovered_duration++;
	else
		global_ui_context.item_hovered_duration = 0;
	
	global_ui_context.item_hovered_id = id;
}

inline void ui_begin(s32 id)
{
	global_ui_context.item_hovered = false;
	global_ui_context.next_id = id * 100;
	global_ui_context.layout.offset_x = 0;
	global_ui_context.layout.offset_y = 0;
	global_ui_context.layout.width = global_ui_context.layout.active_window->width;
	global_ui_context.layout.height = global_ui_context.layout.active_window->height;
	global_ui_context.layout.active_dropdown_state = 0;
	global_ui_context.submenus.count = 0;
	global_ui_context.cursor_to_set = CURSOR_DEFAULT;
}

static void ui_set_cursor(cursor_type type)
{
	if (global_ui_context.cursor_to_set == CURSOR_DEFAULT)
		global_ui_context.cursor_to_set = type;
}

inline void ui_end()
{
	platform_set_cursor(global_ui_context.layout.active_window, global_ui_context.cursor_to_set);
	if (!global_ui_context.item_hovered) global_ui_context.item_hovered_duration = 0;
}

inline submenu_state ui_create_submenu()
{
	submenu_state state;
	state.open = false;
	state.hovered = false;
	state.item_count = 0;
	state.w = MENU_ITEM_WIDTH;
	
	return state;
}

inline checkbox_state ui_create_checkbox(bool selected)
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
	state.buffer = mem_alloc(max_len+1);
	state.buffer[0] = 0;
	state.state = false;
	state.text_offset_x = 0;
	state.history = array_create(sizeof(textbox_history_entry));
	state.future = array_create(sizeof(textbox_history_entry));
	array_reserve(&state.history, 100);
	state.history.reserve_jump = 100;
	array_reserve(&state.future, 100);
	state.future.reserve_jump = 100;
	state.selection_start_index = 0;
	state.double_clicked_to_select = false;
	state.double_clicked_to_select_cursor_index = 0;
	state.diff = 0;
	state.last_click_cursor_index = -1;
	state.attempting_to_select = false;
	state.deselect_on_enter = true;
	state.accept_newline = false;
	
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
	array_destroy(&state->future);
	
	mem_free(state->buffer);
}

inline button_state ui_create_button()
{
	button_state state;
	state.state = 0;
	
	return state;
}

inline scroll_state ui_create_scroll(s32 scroll)
{
	scroll_state state;
	state.scroll = 0;
	state.height = scroll;
	state.mouse_scrolling = false;
	state.in_scroll = false;
	
	return state;
}

void ui_set_textbox_text(textbox_state *textbox, char *text)
{
	if (global_ui_context.current_active_textbox == textbox)
	{
		keyboard_set_input_text(global_ui_context.keyboard, text);
	}
	
	textbox->diff = 0;
	string_copyn(textbox->buffer, text, textbox->max_len);
}

inline dropdown_state ui_create_dropdown()
{
	dropdown_state state;
	state.state = 0;
	state.selected_index = 0;
	return state;
}

void ui_set_style(u16 style)
{
	global_ui_context.style.id = style;
	if (style == UI_STYLE_LIGHT)
	{
		global_ui_context.style.hypertext_foreground = rgb(66, 134, 244);
		global_ui_context.style.hypertext_hover_foreground = rgb(221, 93, 202);
		global_ui_context.style.image_outline_tint = rgb(200,200,200);
		global_ui_context.style.scrollbar_handle_background = rgb(225,225,225);
		global_ui_context.style.info_bar_background = rgb(225,225,225);
		global_ui_context.style.error_foreground = rgb(224,79,95);
		global_ui_context.style.item_hover_background = rgb(240,220,220);
		global_ui_context.style.scrollbar_background = rgb(255,255,255);
		global_ui_context.style.background = rgb(255,255,255);
		global_ui_context.style.menu_hover_background = rgb(200,200,200);
		global_ui_context.style.menu_background = rgb(225,225,225);
		global_ui_context.style.widget_hover_background = rgb(200,200,200);
		global_ui_context.style.widget_background = rgb(225,225,225);
		global_ui_context.style.border = rgb(180,180,180);
		global_ui_context.style.foreground = rgb(10, 10, 10);
		global_ui_context.style.textbox_background = rgb(240,240,240);
		global_ui_context.style.textbox_foreground = rgb(10,10,10);
		global_ui_context.style.textbox_placeholder_foreground = rgb(80,80,80);
		global_ui_context.style.textbox_active_border = rgb(66, 134, 244);
		global_ui_context.style.widget_confirm_background = rgb(211, 80, 80);
		global_ui_context.style.widget_confirm_hover_background = rgb(211, 53, 53);
		global_ui_context.style.widget_confirm_border = rgb(130, 0, 0);
	}
	if (style == UI_STYLE_DARK)
	{
		global_ui_context.style.hypertext_foreground = rgb(66, 134, 244);
		global_ui_context.style.hypertext_hover_foreground = rgb(221, 93, 202);
		global_ui_context.style.scrollbar_handle_background = rgb(50,50,50);
		global_ui_context.style.menu_hover_background = rgb(60,60,60);
		global_ui_context.style.item_hover_background = rgb(80,60,60);
		global_ui_context.style.image_outline_tint = rgb(200,200,200);
		global_ui_context.style.error_foreground = rgb(224,79,95);
		global_ui_context.style.scrollbar_background = rgb(80,80,80);
		global_ui_context.style.widget_hover_background = rgb(50,50,50);
		global_ui_context.style.widget_background = rgb(65,65,65);
		global_ui_context.style.info_bar_background = rgb(65,65,65);
		global_ui_context.style.menu_background = rgb(65,65,65);
		global_ui_context.style.background = rgb(80, 80, 80);
		global_ui_context.style.border = rgb(60,60,60);
		global_ui_context.style.foreground = rgb(240,240,240);
		global_ui_context.style.textbox_background = rgb(65,65,65);
		global_ui_context.style.textbox_foreground = rgb(240, 240,240);
		global_ui_context.style.textbox_active_border = rgb(66, 134, 244);
	}
}

static scroll_state empty_scroll;
inline void ui_create(platform_window *window, keyboard_input *keyboard, mouse_input *mouse, camera *camera, font *font_small)
{
	ui_set_style(UI_STYLE_LIGHT);
	
	global_ui_context.layout.layout_direction = LAYOUT_VERTICAL;
	global_ui_context.layout.offset_x = 0;
	global_ui_context.layout.offset_y = 0;
	global_ui_context.layout.active_window = window;
	global_ui_context.layout.width = global_ui_context.layout.active_window->width;
	empty_scroll = ui_create_scroll(1);
	global_ui_context.layout.scroll = &empty_scroll;
	
	global_ui_context.keyboard = keyboard;
	global_ui_context.mouse = mouse;
	global_ui_context.camera = camera;
	global_ui_context.font_small = font_small;
	global_ui_context.menu_item_count = 0;
	global_ui_context.active_menu_id = -1;
	
	global_ui_context.active_dropdown = 0;
	global_ui_context.confirming_button_id = -1;
	global_ui_context.current_active_textbox = 0;
	
	global_ui_context.item_hovered = false;
	global_ui_context.item_hovered_id = -1;
	global_ui_context.item_hovered_duration = 0;
}

static void ui_pop_scissor()
{
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 w = global_ui_context.layout.scroll->width;
		s32 h = global_ui_context.layout.scroll->height;
		s32 x = global_ui_context.layout.scroll->x;
		s32 y = global_ui_context.layout.scroll->y;
		
		render_set_scissor(global_ui_context.layout.active_window, x,y,w,h);
	}
	else
	{
		render_reset_scissor();
	}
}

inline void ui_block_begin(layout_direction direction)
{
	u32 id = ui_get_id();
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
	
	render_rectangle(0, y, w, MENU_BAR_HEIGHT, global_ui_context.style.menu_background);
	render_rectangle(0, y, w, 1, global_ui_context.style.border);
	global_ui_context.layout.menu_offset_y = 0;
}

inline bool ui_is_menu_active(u32 id)
{
	return id == global_ui_context.active_menu_id;
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

void ui_push_vertical_dragbar()
{
	u32 id = ui_get_id();
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x + global_ui_context.layout.width;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y - WIDGET_PADDING;
	s32 h = global_ui_context.layout.height;
	
	render_rectangle(x, y, 2, h, global_ui_context.style.border);
}

inline void ui_push_menu_item_separator()
{
	global_ui_context.layout.menu_offset_y += 1;
}

static s32 ui_get_scroll()
{
	if (global_ui_context.layout.scroll->in_scroll)
	{
		return global_ui_context.layout.scroll->scroll;
	}
	
	return 0;
}

bool ui_push_color_button(char *text, bool selected, color c)
{
	bool result = false;
	
	u32 id = ui_get_id();
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->px_h/2);
	s32 total_w =
		BUTTON_HORIZONTAL_TEXT_PADDING + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	color bg_color = c;
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		ui_set_cursor(CURSOR_POINTER);
		bg_color.r-=20;
		bg_color.g-=20;
		bg_color.b-=20;
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			result = true;
		}
	}
	
	if (selected)
	{
		render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
		render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 4, global_ui_context.style.border);
	}
	else
	{
		render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
		render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, global_ui_context.style.border);
	}
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

bool ui_push_dropdown_item(image *icon, char *title, s32 index)
{
	bool result = false;
	
	set_render_depth(30);
	
	u32 id = ui_get_id();
	global_ui_context.layout.dropdown_item_count++;
	s32 h = BUTTON_HEIGHT;
	s32 x = global_ui_context.layout.dropdown_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() + ((global_ui_context.layout.dropdown_item_count)*h-(1*global_ui_context.layout.dropdown_item_count));
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->px_h / 2);
	s32 total_w = DROPDOWN_ITEM_WIDTH 
		+ BUTTON_HORIZONTAL_TEXT_PADDING + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	color bg_color = global_ui_context.style.widget_background;
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y > y && mouse_y < y + h)
	{
		ui_set_hovered(id, x,y,total_w,h);
		
		ui_set_cursor(CURSOR_POINTER);
		if (is_left_clicked(global_ui_context.mouse))
		{
			global_ui_context.layout.active_dropdown_state->selected_index = index;
			result = true;
		}
		
		bg_color = global_ui_context.style.widget_hover_background;
	}
	
	
	render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
	render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, global_ui_context.style.border);
	if (icon)
	{
		render_image(icon, x+(BUTTON_HORIZONTAL_TEXT_PADDING/2), 
					 y + (h - (h-6))/2, h-6, h-6);
		text_x += h-10;
	}
	render_text(global_ui_context.font_small, text_x+(BUTTON_HORIZONTAL_TEXT_PADDING/2)-5, text_y, title, global_ui_context.style.foreground);
	
	
#if 0
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
#endif
	set_render_depth(1);
	
	return result;
}

bool ui_push_dropdown(dropdown_state *state, char *title)
{
	bool result = false;
	
	global_ui_context.layout.active_dropdown_state = state;
	
	u32 id = ui_get_id();
	global_ui_context.layout.dropdown_item_count = 0;
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->px_h/2);
	s32 total_w = DROPDOWN_WIDTH + BUTTON_HORIZONTAL_TEXT_PADDING + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	color bg_color = global_ui_context.style.widget_background;
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= y && mouse_y < y + h && !global_ui_context.item_hovered)
	{
		ui_set_hovered(id, x,y,total_w,h);
		
		ui_set_cursor(CURSOR_POINTER);
		if (is_left_clicked(global_ui_context.mouse))
		{
			state->state = !state->state;
		}
		
		bg_color = global_ui_context.style.widget_hover_background;
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
	
	render_triangle(x+total_w - h, y+(h-(h-12))/2, h-12, h-12, global_ui_context.style.border, TRIANGLE_DOWN);
	global_ui_context.layout.dropdown_x = global_ui_context.layout.offset_x;
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result || state->state;
}

bool ui_push_menu(char *title)
{
	bool result = false;
	
	global_ui_context.layout.menu_offset_y = 0;
	global_ui_context.menu_item_count = 0;
	u32 id = ui_get_id();
	
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 w = calculate_text_width(global_ui_context.font_small, title) +
		(MENU_HORIZONTAL_PADDING*2);
	s32 text_h = global_ui_context.font_small->px_h;
	s32 h = MENU_BAR_HEIGHT-1;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y+1;
	s32 text_y = global_ui_context.layout.offset_y - (text_h / 2) + (h / 2) + global_ui_context.camera->y;
	s32 text_x = x + MENU_HORIZONTAL_PADDING;
	
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	color bg_color = global_ui_context.style.menu_background;
	
	bool is_open = ui_is_menu_active(id);
	result = is_open;
	
	if (mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h)
	{
		ui_set_cursor(CURSOR_POINTER);
		if (is_left_clicked(global_ui_context.mouse))
		{
			printf("---------------------------------\n");
			printf("id %d, open: %d\n", id, is_open);
			
			if (is_open)
				global_ui_context.active_menu_id = -1;
			else
				global_ui_context.active_menu_id = id;
			
			result = !is_open;
			is_open = result;
			
			printf("id %d, open: %d\n", id, is_open);
		}
		
		bg_color = global_ui_context.style.menu_hover_background;
	}
	else if (is_left_down(global_ui_context.mouse))
	{
		if (is_open)
			global_ui_context.active_menu_id = -1;
		is_open = false;
	}
	if (!global_ui_context.layout.active_window->has_focus && is_open)
	{
		global_ui_context.active_menu_id = -1;
		is_open = false;
	}
	
	render_rectangle(x, y, w, h, bg_color);
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	
	global_ui_context.layout.prev_offset_x = global_ui_context.layout.offset_x;
	global_ui_context.layout.offset_x += w;
	
	return result;
}

static void ui_set_active_textbox(textbox_state *state)
{
	if (global_ui_context.current_active_textbox && global_ui_context.current_active_textbox != state)
	{
		global_ui_context.current_active_textbox->state = false;
	}
	global_ui_context.current_active_textbox = state;
}

void ui_set_textbox_active(textbox_state *textbox)
{
	ui_set_active_textbox(textbox);
	keyboard_set_input_text(global_ui_context.keyboard, textbox->buffer);
	textbox->state = true;
	global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
	global_ui_context.keyboard->take_input = textbox->state;
}

bool ui_push_textbox(textbox_state *state, char *placeholder)
{
	bool result = false;
	static u64 cursor_tick = 0;
	static u64 last_cursor_pos = -1;
	
	if (!global_ui_context.layout.active_window->has_focus)
		state->state = false;
	
	u32 id = ui_get_id();
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + 5;
	s32 text_y = y + (TEXTBOX_HEIGHT/2) - (global_ui_context.font_small->px_h/2);
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < TEXTBOX_HEIGHT)
		global_ui_context.layout.block_height = TEXTBOX_HEIGHT;
	
	bool has_text = state->buffer[0] != 0;
	
	s32 virt_top = y;
	s32 virt_bottom = y + TEXTBOX_HEIGHT;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	/////////////////////////////////////////////
	/////////////////////////////////////////////
	/////////////////////////////////////////////
	
	bool is_selecting = false;
	bool clicked_to_select = false;
	bool double_clicked_to_select_first = false;
	bool clicked_to_set_cursor = false;
	if (mouse_x >= x && mouse_x < x + TEXTBOX_WIDTH && mouse_y >= virt_top && mouse_y < virt_bottom)
	{
		ui_set_cursor(CURSOR_TEXT);
		
		if (is_left_double_clicked(global_ui_context.mouse) && has_text)
		{
			ui_set_active_textbox(state);
			
			global_ui_context.keyboard->selection_begin_offset = 0;
			global_ui_context.keyboard->selection_length = utf8len(global_ui_context.keyboard->input_text);
			global_ui_context.keyboard->has_selection = true;
			state->selection_start_index = 0;
			
			global_ui_context.mouse->left_state &= ~MOUSE_DOUBLE_CLICK;
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			
			state->double_clicked_to_select = true;
			double_clicked_to_select_first = true;
		}
		if (is_left_clicked(global_ui_context.mouse))
		{
			ui_set_active_textbox(state);
			
			keyboard_set_input_text(global_ui_context.keyboard, state->buffer);
			cursor_tick = 0;
			
			if (global_ui_context.keyboard->has_selection)
			{
				global_ui_context.keyboard->has_selection = false;
			}
			
			clicked_to_set_cursor = true;
			
			state->state = true;
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			result = true;
			
			global_ui_context.keyboard->take_input = state->state;
		}
	}
	else if (is_left_clicked(global_ui_context.mouse))
	{
		if (state->state)
		{
			global_ui_context.keyboard->has_selection = false;
		}
		
		state->state = false;
	}
	
	if (is_left_released(global_ui_context.mouse))
	{
		state->attempting_to_select = false;
	}
	
	if (state->state && global_ui_context.keyboard->has_selection && is_left_down(global_ui_context.mouse))
		is_selecting = true;
	
	if (keyboard_is_key_pressed(global_ui_context.keyboard, KEY_ENTER) && state->deselect_on_enter)
	{
		global_ui_context.keyboard->has_selection = false;
		state->state = false;
	}
	if (state->state && keyboard_is_key_down(global_ui_context.keyboard, KEY_LEFT_CONTROL) &&
		keyboard_is_key_pressed(global_ui_context.keyboard, KEY_ENTER) &&
		state->accept_newline)
	{
		keyboard_handle_input_string(global_ui_context.layout.active_window, global_ui_context.keyboard, "\n");
	}
	
	// calculate scissor rectangle for background
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 scissor_x = x;
		s32 scissor_y = global_ui_context.layout.scroll->y;
		s32 scissor_w = TEXTBOX_WIDTH;
		s32 scissor_h = global_ui_context.layout.scroll->height - 2;
		
		render_set_scissor(global_ui_context.layout.active_window, scissor_x,
						   scissor_y, scissor_w, scissor_h);
	}
	else
	{
		s32 scissor_x = x - global_ui_context.camera->x;
		s32 scissor_y = y - global_ui_context.camera->y;
		s32 scissor_w = TEXTBOX_WIDTH;
		s32 scissor_h = TEXTBOX_HEIGHT;
		
		render_set_scissor(global_ui_context.layout.active_window, 
						   scissor_x, scissor_y, scissor_w, scissor_h);
	}
	
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
	
	// calculate scissor rectangle for text
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 scissor_x = x+3;
		s32 scissor_y = global_ui_context.layout.scroll->y;
		s32 scissor_w = TEXTBOX_WIDTH-5;
		s32 scissor_h = global_ui_context.layout.scroll->height - 2;
		
		render_set_scissor(global_ui_context.layout.active_window, scissor_x,
						   scissor_y, scissor_w, scissor_h);
	}
	else
	{
		s32 scissor_x = x - global_ui_context.camera->x+3;
		s32 scissor_y = y - global_ui_context.camera->y;
		s32 scissor_w = TEXTBOX_WIDTH-5;
		s32 scissor_h = TEXTBOX_HEIGHT;
		
		render_set_scissor(global_ui_context.layout.active_window, 
						   scissor_x, scissor_y, scissor_w, scissor_h);
	}
	
	s32 cursor_text_w;
	
	// select first character on click
	if (clicked_to_set_cursor)
	{
		global_ui_context.keyboard->cursor = calculate_cursor_position(global_ui_context.font_small, 
																	   state->buffer, mouse_x + state->diff - text_x);
		
		state->last_click_cursor_index = global_ui_context.keyboard->cursor;
		state->attempting_to_select = true;
		
		global_ui_context.keyboard->selection_begin_offset = global_ui_context.keyboard->cursor;
		
#if 0
		global_ui_context.keyboard->has_selection = true;
		global_ui_context.keyboard->selection_begin_offset = calculate_cursor_position(global_ui_context.font_small, 
																					   state->buffer, mouse_x + state->diff - text_x);
		global_ui_context.keyboard->selection_length = 1;
		state->selection_start_index = global_ui_context.keyboard->selection_begin_offset;
#endif
	}
	
	if (state->state)
	{
		s32 len = utf8len(global_ui_context.keyboard->input_text);
		s32 old_len = utf8len(state->buffer);
		
		// check if text changes, add to history if true
		bool is_lctrl_down = global_ui_context.keyboard->keys[KEY_LEFT_CONTROL];
		
		// go to previous state
		if (is_lctrl_down && keyboard_is_key_pressed(global_ui_context.keyboard, KEY_Z) && state->history.length)
		{
			textbox_history_entry history_entry;
			history_entry.text = mem_alloc(strlen(state->buffer)+1);
			history_entry.cursor_offset = last_cursor_pos;
			string_copyn(history_entry.text, state->buffer, MAX_INPUT_LENGTH);
			array_push(&state->future, &history_entry);
			
			global_ui_context.keyboard->text_changed = true;
			
			textbox_history_entry *old_text = array_at(&state->history, state->history.length-1);
			string_copyn(state->buffer, old_text->text, MAX_INPUT_LENGTH);
			keyboard_set_input_text(global_ui_context.keyboard, state->buffer);
			
			mem_free(old_text->text);
			array_remove_at(&state->history, state->history.length-1);
			
			global_ui_context.keyboard->cursor = old_text->cursor_offset;
		}
		else if (is_lctrl_down && 
				 keyboard_is_key_pressed(global_ui_context.keyboard, KEY_Y) && state->future.length)
		{
			textbox_history_entry history_entry;
			history_entry.text = mem_alloc(strlen(state->buffer)+1);
			history_entry.cursor_offset = last_cursor_pos;
			string_copyn(history_entry.text, state->buffer, MAX_INPUT_LENGTH);
			array_push(&state->history, &history_entry);
			
			global_ui_context.keyboard->text_changed = true;
			
			textbox_history_entry *old_text = array_at(&state->future, state->future.length-1);
			string_copyn(state->buffer, old_text->text, MAX_INPUT_LENGTH);
			keyboard_set_input_text(global_ui_context.keyboard, state->buffer);
			
			mem_free(old_text->text);
			array_remove_at(&state->future, state->future.length-1);
			
			global_ui_context.keyboard->cursor = old_text->cursor_offset;
		}
		else
		{
			if (global_ui_context.keyboard->text_changed)
			{
				if (last_cursor_pos != -1)
				{
					textbox_history_entry history_entry;
					history_entry.text = mem_alloc(strlen(state->buffer)+1);
					history_entry.cursor_offset = last_cursor_pos;
					string_copyn(history_entry.text, state->buffer, MAX_INPUT_LENGTH);
					array_push(&state->history, &history_entry);
				}
			}
			
			string_copyn(state->buffer, global_ui_context.keyboard->input_text, state->max_len);
			
			if (global_ui_context.keyboard->cursor > state->max_len)
			{
				global_ui_context.keyboard->cursor = state->max_len;
				utf8_str_replace_at(global_ui_context.keyboard->input_text,global_ui_context.keyboard->cursor, 0);
			}
		}
		
		// cursor ticking after text change
		if (last_cursor_pos != global_ui_context.keyboard->cursor || global_ui_context.keyboard->text_changed)
			cursor_tick = 0;
		
		// draw cursor
		cursor_text_w = calculate_text_width_upto(global_ui_context.font_small, 
												  state->buffer, global_ui_context.keyboard->cursor);
		
		s32 text_w = calculate_text_width(global_ui_context.font_small, state->buffer);
		
#if 1
		// change offset after cursor position change
		if (!is_selecting && !global_ui_context.keyboard->has_selection && !state->attempting_to_select)
		{
			if (cursor_text_w < state->diff)
			{
				state->diff = cursor_text_w;
			}
			if (cursor_text_w - state->diff > TEXTBOX_WIDTH - 10)
			{
				state->diff = (cursor_text_w) - (TEXTBOX_WIDTH - 10);
			}
		}
#endif
		
		// make sure offset is recalculated when text changes or a portion of text is changed so the textbox doesnt end up half empty
#if 1
		if (!clicked_to_select && !clicked_to_set_cursor && !is_selecting && !global_ui_context.keyboard->has_selection && global_ui_context.keyboard->text_changed)
		{
			s32 max_offset_x = text_w - (TEXTBOX_WIDTH-10);
			
			if (state->diff > max_offset_x && max_offset_x > 0)
				state->diff = max_offset_x;
			else if ((cursor_text_w <= TEXTBOX_WIDTH -10))
				state->diff = 0;
			
#if 0
			if ((state->diff+cursor_text_w > TEXTBOX_WIDTH -10) && (global_ui_context.keyboard->text_changed || global_ui_context.keyboard->cursor != last_cursor_pos))
			{
				state->diff = cursor_text_w - TEXTBOX_WIDTH + 10;
			}
			else if ((state->diff+cursor_text_w <= TEXTBOX_WIDTH -10))
			{
				state->diff = 0;
			}
#endif
		}
#endif
	}
	
	s32 curr_index = calculate_cursor_position(global_ui_context.font_small, 
											   state->buffer, mouse_x + state->diff - text_x);
	
	//////////////////////////////////
	{
		if (curr_index != state->last_click_cursor_index && state->attempting_to_select)
		{
			clicked_to_select = true;
			state->attempting_to_select = false;
		}
	}
	
	
	// select first character on click
	if (clicked_to_select)
	{
#if 1
		global_ui_context.keyboard->has_selection = true;
		//global_ui_context.keyboard->selection_begin_offset = calculate_cursor_position(global_ui_context.font_small, 
		//state->buffer, mouse_x + state->diff - text_x);
		global_ui_context.keyboard->selection_length = 0;
		state->selection_start_index = global_ui_context.keyboard->selection_begin_offset;
		
		state->selection_start_index--;
#endif
	}
	
	
#if 1
	if (is_selecting)
	{
		// move text offset x when selecting so we can select more text than available on screen.
		if (global_ui_context.mouse->x < x + 10)
		{
			s32 text_w = calculate_text_width(global_ui_context.font_small, state->buffer);
			if (text_w > TEXTBOX_WIDTH-10)
			{
				state->diff -= TEXTBOX_SCROLL_X_SPEED;
				if (state->diff < 0) state->diff = 0;
			}
		}
		if (global_ui_context.mouse->x > x + TEXTBOX_WIDTH - 10)
		{
			s32 text_w = calculate_text_width(global_ui_context.font_small, state->buffer);
			s32 diff = text_w - TEXTBOX_WIDTH + 10;
			
			if (text_w > TEXTBOX_WIDTH-10)
			{
				state->diff += TEXTBOX_SCROLL_X_SPEED;
				if (state->diff > diff)
					state->diff = diff;
			}
		}
		///////////////////////////////////////////////////////////
	}
#endif
	
	// change selection area based on cursor position.
	// if double clicked to select the entire textbox we should only 
	// do this when the mouse has moved enough to select a new character
#if 1
	if (is_selecting)
	{
		s32 index = calculate_cursor_position(global_ui_context.font_small, 
											  state->buffer, mouse_x + state->diff - text_x)+1;
		
		if (double_clicked_to_select_first)
			state->double_clicked_to_select_cursor_index = index;
		
		if (!state->double_clicked_to_select || (state->double_clicked_to_select && index != state->double_clicked_to_select_cursor_index))
		{
			if (index <= state->selection_start_index+1)
			{
				global_ui_context.keyboard->selection_begin_offset = index - 1;
				global_ui_context.keyboard->selection_length = state->selection_start_index - index + 2;
			}
			else if (index > state->selection_start_index)
			{
				global_ui_context.keyboard->selection_begin_offset = state->selection_start_index+1;
				global_ui_context.keyboard->selection_length = index - state->selection_start_index-2;
			}
			
			state->double_clicked_to_select = false;
		}
	}
#endif
	
	if (state->state)
	{
		last_cursor_pos = global_ui_context.keyboard->cursor;
	}
	
	if (!has_text)
	{
		if (!state->state)
			render_text(global_ui_context.font_small, text_x - state->diff, text_y, 
						placeholder, global_ui_context.style.textbox_placeholder_foreground);
		else
			render_text_with_cursor(global_ui_context.font_small, text_x - state->diff, text_y, 
									placeholder, global_ui_context.style.textbox_placeholder_foreground, global_ui_context.keyboard->cursor);
	}
	else
	{
		if (global_ui_context.keyboard->has_selection && state->state && global_ui_context.keyboard->selection_length)
			render_text_with_selection(global_ui_context.font_small, text_x - state->diff, text_y, 
									   state->buffer, global_ui_context.style.foreground, global_ui_context.keyboard->selection_begin_offset,
									   global_ui_context.keyboard->selection_length);
		else if (state->state)
			render_text_with_cursor(global_ui_context.font_small, text_x - state->diff, text_y, 
									state->buffer, global_ui_context.style.foreground, global_ui_context.keyboard->cursor);
		else
			render_text(global_ui_context.font_small, text_x - state->diff, text_y, 
						state->buffer, global_ui_context.style.foreground);
	}
	
	ui_pop_scissor();
	//render_reset_scissor();
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += TEXTBOX_WIDTH + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += TEXTBOX_HEIGHT + WIDGET_PADDING;
	
	return result || state->state;
}

bool ui_push_hypertext_link(char *text)
{
	bool result = false;
	
	u32 id = ui_get_id();
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + WIDGET_PADDING;
	s32 text_h = global_ui_context.font_small->px_h + 10;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (global_ui_context.font_small->px_h/2) + spacing_y;
	s32 total_w = calculate_text_width(global_ui_context.font_small, text) +
		WIDGET_PADDING + WIDGET_PADDING + 10;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < global_ui_context.font_small->px_h)
		global_ui_context.layout.block_height = global_ui_context.font_small->px_h;
	
	color bg_color = global_ui_context.style.hypertext_foreground;
	bool hovered = false;
	if (mouse_x >= text_x && mouse_x < text_x + total_w && mouse_y >= text_y && mouse_y < text_y+text_h && !global_ui_context.item_hovered)
	{
		hovered = true;
		if (is_left_clicked(global_ui_context.mouse))
		{
			result = true;
		}
		bg_color = global_ui_context.style.hypertext_hover_foreground;
	}
	
	s32 text_width = render_text(global_ui_context.font_small, text_x + 5, text_y + 5, text, bg_color);
	
	if (result)
		render_rectangle(text_x, text_y+text_h+2, text_width+10, 1, bg_color);
	else if (hovered)
		render_rectangle(text_x, text_y+text_h, text_width+10, 1, bg_color);
	
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += CHECKBOX_SIZE + WIDGET_PADDING;
	
	return result;
}

void ui_push_textf(font *f, char *text)
{
	u32 id = ui_get_id();
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + WIDGET_PADDING;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (f->px_h/2) + spacing_y;
	s32 total_w = calculate_text_width(f, text) +
		WIDGET_PADDING + WIDGET_PADDING;
	
	if (global_ui_context.layout.block_height < f->px_h)
		global_ui_context.layout.block_height = f->px_h+5;
	
	render_text(f, text_x, text_y, text, global_ui_context.style.foreground);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += CHECKBOX_SIZE + WIDGET_PADDING;
}


void ui_push_textf_width(font *f, char *text, s32 maxw)
{
	u32 id = ui_get_id();
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + WIDGET_PADDING;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (f->px_h/2) + spacing_y;
	s32 total_w = maxw +
		WIDGET_PADDING + WIDGET_PADDING;
	maxw -= (WIDGET_PADDING*2);
	if (global_ui_context.layout.block_height < f->px_h)
		global_ui_context.layout.block_height = f->px_h+5;
	
	render_text_ellipsed(f, text_x, text_y, maxw, text, global_ui_context.style.foreground);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += CHECKBOX_SIZE + WIDGET_PADDING;
}


void ui_push_text(char *text)
{
	u32 id = ui_get_id();
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + WIDGET_PADDING;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (global_ui_context.font_small->px_h/2) + spacing_y;
	s32 total_w = calculate_text_width(global_ui_context.font_small, text) +
		WIDGET_PADDING + WIDGET_PADDING;
	
	if (global_ui_context.layout.block_height < global_ui_context.font_small->px_h)
		global_ui_context.layout.block_height = global_ui_context.font_small->px_h+5;
	
	render_text(global_ui_context.font_small, text_x, text_y, text, global_ui_context.style.foreground);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += CHECKBOX_SIZE + WIDGET_PADDING;
}

void ui_push_rect(s32 w, color c)
{
	u32 id = ui_get_id();
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 total_w = w +
		WIDGET_PADDING + WIDGET_PADDING;
	s32 h = BUTTON_HEIGHT;
	
	s32 virt_top = y;
	s32 virt_bottom = y + TEXTBOX_HEIGHT;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	{
		render_rectangle(x+WIDGET_PADDING,y,w,h,c);
	}
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
}

bool ui_push_text_width(char *text, s32 maxw, bool active)
{
	bool result = false;
	
	u32 id = ui_get_id();
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + WIDGET_PADDING;
	s32 h = BUTTON_HEIGHT;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (global_ui_context.font_small->px_h/2) + spacing_y;
	s32 total_w = maxw +
		WIDGET_PADDING + WIDGET_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (global_ui_context.layout.block_height < global_ui_context.font_small->px_h)
		global_ui_context.layout.block_height = global_ui_context.font_small->px_h+5;
	maxw -= (WIDGET_PADDING*2);
	if (active)
	{
		bool hovered = false;
		if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
		{
			hovered = true;
			ui_set_cursor(CURSOR_POINTER);
			if (is_left_clicked(global_ui_context.mouse))
			{
				result = true;
			}
		}
		
		if (hovered)
		{
			render_rectangle_outline(x-1,y+spacing_y,total_w, h, 1, global_ui_context.style.textbox_active_border);
		}
	}
	
	render_text_ellipsed(global_ui_context.font_small, text_x, text_y, maxw, text, global_ui_context.style.foreground);
	
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

bool ui_push_checkbox(checkbox_state *state, char *title)
{
	bool result = false;
	
	u32 id = ui_get_id();
	s32 spacing_y = (BLOCK_HEIGHT - CHECKBOX_SIZE)/2;
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll() - spacing_y;
	s32 text_x = x + CHECKBOX_SIZE + WIDGET_PADDING;
	s32 text_y = y + (BLOCK_HEIGHT/2) - (global_ui_context.font_small->px_h/2) + spacing_y;
	s32 total_w = calculate_text_width(global_ui_context.font_small, title) +
		CHECKBOX_SIZE + WIDGET_PADDING + WIDGET_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < CHECKBOX_SIZE)
		global_ui_context.layout.block_height = CHECKBOX_SIZE;
	
	render_rectangle_outline(x, y, CHECKBOX_SIZE, CHECKBOX_SIZE, 1, global_ui_context.style.border);
	
	s32 virt_top = y;
	s32 virt_bottom = y + CHECKBOX_SIZE;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		ui_set_hovered(id, x,y,total_w,CHECKBOX_SIZE);
		
		ui_set_cursor(CURSOR_POINTER);
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

inline bool is_shortcut_down(s32 shortcut_keys[2])
{
	return keyboard_is_key_down(global_ui_context.keyboard, shortcut_keys[0]) &&
		keyboard_is_key_pressed(global_ui_context.keyboard, shortcut_keys[1]);
}

void ui_begin_menu_submenu(submenu_state *state, char *title)
{
	bool result = ui_push_menu_item(title, "");
	
	s32 w = state->w;
	s32 h = MENU_BAR_HEIGHT;
	s32 x = global_ui_context.layout.prev_offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + (global_ui_context.menu_item_count * h)+1 +
		global_ui_context.layout.menu_offset_y + global_ui_context.camera->y;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	state->x = x + MENU_ITEM_WIDTH;
	state->y = y;
	state->item_count = 0;
	
	if ((mouse_x >= x && mouse_x < x + MENU_ITEM_WIDTH && mouse_y >= y && mouse_y < y + h))
	{
		state->hovered = true;
		state->open = true;
	}
	else
	{
		state->hovered = false;
	}
	
	assert(global_ui_context.submenus.count <= 4);
	global_ui_context.submenus.submenu_stack[global_ui_context.submenus.count++] = state;
	
	if (result) state->open = false;
}

void ui_end_menu_submenu(char* empty_placeholder)
{
	set_render_depth(30);
	
	submenu_state *state = global_ui_context.submenus.submenu_stack[global_ui_context.submenus.count-1];
	
	s32 w = state->w;
	s32 h = MENU_BAR_HEIGHT;
	s32 total_h = state->item_count*h;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (state->open)
	{
		if (!(mouse_x >= state->x && mouse_x < state->x + w && mouse_y >= state->y && mouse_y < state->y + total_h) && !state->hovered)
		{
			state->open = false;
		}
		
		if (!state->item_count)
		{
			s32 text_h = global_ui_context.font_small->px_h;
			s32 text_y = state->y - (text_h / 2) + (h / 2);
			s32 text_x = state->x + MENU_HORIZONTAL_PADDING;
			
			total_h = h;
			render_rectangle(state->x, state->y, w, total_h, global_ui_context.style.widget_background);
			render_rectangle_outline(state->x, state->y, w, total_h, 1, global_ui_context.style.border);
			
			render_text(global_ui_context.font_small, text_x, text_y, empty_placeholder, global_ui_context.style.foreground);
		}
		else
		{
			render_rectangle(state->x, state->y, w, 1, global_ui_context.style.border);
		}
	}
	
	
	global_ui_context.submenus.count--;
	
	set_render_depth(1);
}

bool ui_push_menu_item(char *title, char *shortcut)
{
	bool result = false;
	
	set_render_depth(30);
	
	s32 x = global_ui_context.layout.prev_offset_x + global_ui_context.camera->x;
	s32 w = MENU_ITEM_WIDTH;
	s32 text_h = global_ui_context.font_small->px_h;
	s32 h = MENU_BAR_HEIGHT;
	s32 y = global_ui_context.layout.offset_y + ((global_ui_context.menu_item_count+1) * h)+1 +
		global_ui_context.layout.menu_offset_y + global_ui_context.camera->y;
	
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	color bg_color = global_ui_context.style.menu_background;
	
	s32 text_y = 0;
	s32 text_x = 0;
	s32 text_2_x = 0;
	
	submenu_state *state = 0;
	if (global_ui_context.submenus.count)
	{
		state = global_ui_context.submenus.submenu_stack[global_ui_context.submenus.count-1];
		w = state->w;
		
		if (!state->open) return false;
		
		x = state->x;
		y = state->y + (state->item_count*h);
		
		text_y = y - (text_h / 2) + (h / 2);
		text_x = x + MENU_HORIZONTAL_PADDING;
		text_2_x = x + w - MENU_HORIZONTAL_PADDING
			- calculate_text_width(global_ui_context.font_small, shortcut);
		
		state->item_count++;
	}
	else
	{
		global_ui_context.menu_item_count++;
		
		text_y = y - (text_h / 2) + (h / 2);
		text_x = x + MENU_HORIZONTAL_PADDING;
		text_2_x = x + w - MENU_HORIZONTAL_PADDING
			- calculate_text_width(global_ui_context.font_small, shortcut);
	}
	
	if ((mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h))
	{
		ui_set_cursor(CURSOR_POINTER);
		bg_color = global_ui_context.style.menu_hover_background;
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			if (state) state->open = false;
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
	
	// shadow
	render_rectangle(x+w, y, 3, MENU_BAR_HEIGHT, rgba(0,0,0,100));
	render_rectangle(x+3, y+h, w, 3, rgba(0,0,0,100));
	
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	render_text(global_ui_context.font_small, text_2_x, text_y, shortcut, global_ui_context.style.foreground);
	
	set_render_depth(1);
	
	return result;
}

bool ui_push_image(image *img, s32 w, s32 h, s32 outline, color tint)
{
	bool result = false;
	
	if (!img->loaded) return result;
	
	u32 id = ui_get_id();
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 total_w = w;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		ui_set_cursor(CURSOR_POINTER);
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
		}
		if (is_left_released(global_ui_context.mouse))
		{
			result = true;
		}
	}
	
	render_image_tint(img,x,y,w,h,global_ui_context.style.image_outline_tint);
	render_image_tint(img,x+outline,y+outline,w-(outline*2),h-(outline*2),tint);
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

bool ui_push_button(button_state *state, char *title)
{
	bool result = false;
	
	u32 id = ui_get_id();
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->px_h/2);
	s32 total_w = calculate_text_width(global_ui_context.font_small, title) +
		BUTTON_HORIZONTAL_TEXT_PADDING + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	color bg_color = global_ui_context.style.widget_background;
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		ui_set_hovered(id, x,y,total_w,h);
		ui_set_cursor(CURSOR_POINTER);
		bg_color = global_ui_context.style.widget_hover_background;
		
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

bool ui_push_button_image_with_confirmation(button_state *state, char *title, image *img)
{
	bool result = false;
	
	u32 id = ui_get_id();
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->px_h/2);
	s32 text_w = calculate_text_width(global_ui_context.font_small, title);
	s32 total_w = text_w + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	bool confirming = global_ui_context.confirming_button_id == id;
	
	if (title[0] == 0)
	{
		total_w = 0;
	}
	
	color bg_color;
	color border_color;
	
	if (confirming)
	{
		bg_color = global_ui_context.style.widget_confirm_background;
		border_color = global_ui_context.style.widget_confirm_border;
	}
	else
	{
		bg_color = global_ui_context.style.widget_background;
		border_color = global_ui_context.style.border;
	}
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	int icon_w = 14;
	int icon_h = 14;
	if (img->loaded)
	{
		float max_icon_size = BUTTON_HEIGHT - (BUTTON_IMAGE_PADDING*2);
		float scale = 1.0f;
		if (img->width >= img->height)
		{
			scale = img->width / max_icon_size;
			
			icon_w = img->width / scale;
			icon_h = icon_w;
		}
		else if (img->height >= img->width)
		{
			scale = img->height / max_icon_size;
			
			icon_h = img->height / scale;
			icon_w = icon_h;
		}
	}
	total_w += icon_w + (BUTTON_IMAGE_SPACING*2);
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		ui_set_hovered(id, x,y,total_w,h);
		ui_set_cursor(CURSOR_POINTER);
		
		if (confirming)
			bg_color = global_ui_context.style.widget_confirm_hover_background;
		else
			bg_color = global_ui_context.style.widget_hover_background;
		
		if (is_left_clicked(global_ui_context.mouse)) 
		{
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			state->state = 1;
			
			if (confirming)
			{
				result = true;
				global_ui_context.confirming_button_id = -1;
			}
		}
		if (is_left_released(global_ui_context.mouse) && (state->state || confirming))
		{
			state->state = 0;
			
			if (!confirming)
			{
				global_ui_context.confirming_button_id = id;
			}
		}
	}
	else
	{
		if (is_left_clicked(global_ui_context.mouse) && confirming) 
		{
			global_ui_context.confirming_button_id = -1;
		}
	}
	
	if (is_left_released(global_ui_context.mouse))
	{
		state->state = 0;
	}
	
	render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
	render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, border_color);
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	
	if (img && img->loaded)
		render_image(img, x + total_w - icon_w - BUTTON_IMAGE_SPACING, y + BUTTON_IMAGE_PADDING, img->width, img->height);
	else
		render_rectangle(x + total_w - icon_w - BUTTON_IMAGE_SPACING, y + BUTTON_IMAGE_PADDING, icon_w, icon_w, rgb(160,160,160));
	
	if (global_ui_context.layout.layout_direction == LAYOUT_HORIZONTAL)
		global_ui_context.layout.offset_x += total_w + WIDGET_PADDING;
	else
		global_ui_context.layout.offset_y += BUTTON_HEIGHT + WIDGET_PADDING;
	
	return result;
}

bool ui_push_button_image(button_state *state, char *title, image *img)
{
	bool result = false;
	
	u32 id = ui_get_id();
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 text_y = y + (BUTTON_HEIGHT/2) - (global_ui_context.font_small->px_h/2);
	s32 text_w = calculate_text_width(global_ui_context.font_small, title);
	s32 total_w = text_w + BUTTON_HORIZONTAL_TEXT_PADDING;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	s32 h = BUTTON_HEIGHT;
	
	if (title[0] == 0)
	{
		total_w = 0;
	}
	
	color bg_color = global_ui_context.style.widget_background;
	
	if (global_ui_context.layout.block_height < h)
		global_ui_context.layout.block_height = h;
	
	int icon_w = 14;
	int icon_h = 14;
	if (img->loaded)
	{
		float max_icon_size = BUTTON_HEIGHT - (BUTTON_IMAGE_PADDING*2);
		float scale = 1.0f;
		if (img->width >= img->height)
		{
			scale = img->width / max_icon_size;
			
			icon_w = img->width / scale;
			icon_h = icon_w;
		}
		else if (img->height >= img->width)
		{
			scale = img->height / max_icon_size;
			
			icon_h = img->height / scale;
			icon_w = icon_h;
		}
	}
	total_w += icon_w + (BUTTON_IMAGE_SPACING*2);
	
	s32 virt_top = y;
	s32 virt_bottom = y + h;
	if (global_ui_context.layout.scroll->in_scroll)
	{
		s32 bottom = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
		if (bottom < virt_bottom)
			virt_bottom = bottom;
		s32 top = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		if (top > virt_top)
			virt_top = top;
	}
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom && !global_ui_context.item_hovered)
	{
		ui_set_hovered(id, x,y,total_w,h);
		
		ui_set_cursor(CURSOR_POINTER);
		bg_color = global_ui_context.style.widget_hover_background;
		
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
	else if (is_left_released(global_ui_context.mouse))
	{
		state->state = 0;
	}
	
	render_rectangle(x, y, total_w, BUTTON_HEIGHT, bg_color);
	render_rectangle_outline(x, y, total_w, BUTTON_HEIGHT, 1, global_ui_context.style.border);
	render_text(global_ui_context.font_small, text_x, text_y, title, global_ui_context.style.foreground);
	
	if (img && img->loaded)
		render_image(img, x + total_w - icon_w - BUTTON_IMAGE_SPACING, y + BUTTON_IMAGE_PADDING, img->width, img->height);
	else
		render_rectangle(x + total_w - icon_w - BUTTON_IMAGE_SPACING, y + BUTTON_IMAGE_PADDING, icon_w, icon_w, rgb(160,160,160));
	
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
	
	//ui_push_separator();
}

inline void ui_destroy()
{
}

void ui_scroll_begin(scroll_state *state)
{
	global_ui_context.layout.scroll = state;
	global_ui_context.layout.scroll->in_scroll = true;
	//global_ui_context.layout.scroll->height = height;
	
	s32 w = global_ui_context.layout.width;
	s32 h = state->height;
	s32 x = global_ui_context.layout.offset_x + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y - WIDGET_PADDING;
	
	state->width = w;
	state->height = h;
	state->x = x;
	state->y = y;
	
	//global_ui_context.layout.offset_x += WIDGET_PADDING;
	global_ui_context.layout.start_offset_x = global_ui_context.layout.offset_x;
	//global_ui_context.layout.offset_y += WIDGET_PADDING;
	global_ui_context.layout.scroll->scroll_start_offset_y = global_ui_context.layout.offset_y;
	
	//render_rectangle_outline(x, y, w, h, 1, global_ui_context.style.border);
	render_set_scissor(global_ui_context.layout.active_window, x, y, w, h);
}

void ui_scroll_end()
{
	s32 max_scroll = (global_ui_context.layout.scroll->scroll_start_offset_y -
					  global_ui_context.layout.offset_y) + global_ui_context.layout.scroll->height - WIDGET_PADDING;
	
	//global_ui_context.layout.offset_x -= WIDGET_PADDING;
	global_ui_context.layout.offset_y = global_ui_context.layout.scroll->scroll_start_offset_y + global_ui_context.layout.scroll->height;
	global_ui_context.layout.offset_y += WIDGET_PADDING;
	
	// draw scrollbar
	if (max_scroll < 0)
	{
		s32 scroll_w = 14;
		
		if (global_ui_context.mouse->x >= global_ui_context.layout.scroll->x &&
			global_ui_context.mouse->x <= global_ui_context.layout.scroll->x+global_ui_context.layout.scroll->width &&
			global_ui_context.mouse->y >= global_ui_context.layout.scroll->y &&
			global_ui_context.mouse->y <= global_ui_context.layout.scroll->y+global_ui_context.layout.scroll->height)
		{
			s32 scroll_y = 0;
			if (global_ui_context.mouse->scroll_state == SCROLL_UP)
				scroll_y+=SCROLL_SPEED;
			if (global_ui_context.mouse->scroll_state == SCROLL_DOWN)
				scroll_y-=SCROLL_SPEED;
			global_ui_context.layout.scroll->scroll += scroll_y;
		}
		
		if (global_ui_context.layout.scroll->scroll > 0)
			global_ui_context.layout.scroll->scroll = 0;
		if (global_ui_context.layout.scroll->scroll < max_scroll)
			global_ui_context.layout.scroll->scroll = max_scroll;
		
		float percentage = global_ui_context.layout.scroll->scroll / 
			(float)max_scroll;
		
		float scrollbar_height_percentage = -(max_scroll - global_ui_context.layout.scroll->height) / (float)global_ui_context.layout.scroll->height;
		
		s32 scrollbar_height = (global_ui_context.layout.scroll->height / scrollbar_height_percentage);
		s32 scrollbar_pos_y = global_ui_context.layout.scroll->scroll_start_offset_y - WIDGET_PADDING;
		
		s32 tw = global_ui_context.layout.width - WIDGET_PADDING*2;
		s32 tx = global_ui_context.layout.offset_x + global_ui_context.camera->x + WIDGET_PADDING;
		
		s32 scrollbar_pos_x = tx + tw + WIDGET_PADDING - scroll_w;
		
		scrollbar_pos_y += (global_ui_context.layout.scroll->height - 
							scrollbar_height) * percentage;
		if (is_left_clicked(global_ui_context.mouse) &&
			global_ui_context.mouse->x >= scrollbar_pos_x && global_ui_context.mouse->x <= scrollbar_pos_x + scroll_w &&
			global_ui_context.mouse->y >= global_ui_context.layout.scroll->y && global_ui_context.mouse->y <= global_ui_context.layout.scroll->y + global_ui_context.layout.scroll->height)
		{
			global_ui_context.layout.scroll->mouse_scrolling = true;
		}
		else if (is_left_released(global_ui_context.mouse))
		{
			global_ui_context.layout.scroll->mouse_scrolling = false;
		}
		//render_reset_scissor();
		if (global_ui_context.layout.scroll->mouse_scrolling)
		{
			float mouse_percentage = (global_ui_context.mouse->y-global_ui_context.layout.scroll->y-(scrollbar_height/2))/
				(float)(global_ui_context.layout.scroll->height-(scrollbar_height));
			
			global_ui_context.layout.scroll->scroll = (s32)(mouse_percentage * max_scroll);
			if (global_ui_context.layout.scroll->scroll > 0) global_ui_context.layout.scroll->scroll = 0;
			if (global_ui_context.layout.scroll->scroll < max_scroll)
				global_ui_context.layout.scroll->scroll = max_scroll;
		}
		
		{
			// scroll background
			render_rectangle(scrollbar_pos_x,global_ui_context.layout.scroll->y,
							 scroll_w,global_ui_context.layout.scroll->height,global_ui_context.style.scrollbar_background);
			
			render_rectangle_outline(scrollbar_pos_x,global_ui_context.layout.scroll->y-1,
									 scroll_w,global_ui_context.layout.scroll->height+2, 1,
									 global_ui_context.style.border);
			
			// scrollbar
			render_rectangle(scrollbar_pos_x, scrollbar_pos_y-1,
							 scroll_w,scrollbar_height+2,global_ui_context.style.scrollbar_handle_background);
			
			render_rectangle_outline(scrollbar_pos_x, scrollbar_pos_y-1,
									 scroll_w,scrollbar_height+2, 1,
									 global_ui_context.style.border);
			
			//render_rectangle(scrollbar_pos_x, scrollbar_pos_y, 10, scrollbar_height, global_ui_context.style.scrollbar_handle_background);
		}
	}
	render_reset_scissor();
	global_ui_context.layout.scroll->in_scroll = false;
}

void ui_push_tooltip(char *text)
{
	if (global_ui_context.item_hovered && global_ui_context.item_hovered_id == global_ui_context.next_id-1)
	{
		if (global_ui_context.item_hovered_duration > (1000/TARGET_FRAMERATE)/2)
		{
			s32 total_w = calculate_text_width(global_ui_context.font_small, text) +
				WIDGET_PADDING + WIDGET_PADDING;
			
			s32 x = 0;
			s32 y = 0;
			s32 triangle_s = 20;
			
			render_reset_scissor();
			
			set_render_depth(30);
			
			// align right
			if (global_ui_context.tooltip.x < (total_w/2))
			{
				x = global_ui_context.tooltip.x + global_ui_context.tooltip.w + WIDGET_PADDING+3;
				y = global_ui_context.tooltip.y;
				
				render_triangle(x-9, y+(TEXTBOX_HEIGHT/2)-9,triangle_s,triangle_s, rgb(40,40,40), TRIANGLE_LEFT);
			}
			// align left
			else if (global_ui_context.tooltip.x > 
					 global_ui_context.layout.active_window->width-(total_w/2))
			{
				assert(0 && "not implemented"); // TODO(Aldrik): implement
			}
			// align bottom
			else
			{
				x = global_ui_context.tooltip.x - (total_w/2) + (global_ui_context.tooltip.w/2);
				y = global_ui_context.tooltip.y + global_ui_context.tooltip.h + 
					WIDGET_PADDING+3;
				
				render_triangle(x+(total_w/2)-(triangle_s/2), y-9,triangle_s,triangle_s, rgb(40,40,40), TRIANGLE_UP);
			}
			
			render_rectangle(x, y,total_w,TEXTBOX_HEIGHT, rgb(40,40,40));
			
			render_text(global_ui_context.font_small, x + WIDGET_PADDING, y + (TEXTBOX_HEIGHT/2)- (global_ui_context.font_small->px_h/2), text, rgb(240,240,240));
			
			set_render_depth(1);
			
			ui_pop_scissor();
		}
	}
}