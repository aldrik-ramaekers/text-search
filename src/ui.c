inline void ui_begin()
{
	global_ui_context.next_id = 0;
	global_ui_context.layout.offset_x = 0;
	global_ui_context.layout.offset_y = 0;
}

inline void ui_end()
{
	
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
	state.buffer = malloc(max_len);
	state.buffer[0] = 0;
	state.state = false;
	state.text_offset_x = 0;
	
	return state;
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

inline bool ui_is_menu_active(u32 id)
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

bool ui_push_menu(char *title)
{
	bool result = false;
	
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
	
	bool is_open = ui_is_menu_active(id);
	result = is_open;
	
	if (mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h)
	{
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

bool ui_push_textbox(textbox_state *state, char *placeholder)
{
	bool result = false;
	static u64 cursor_tick = 0;
	static u64 last_cursor_pos = 0;
	
	s32 x = global_ui_context.layout.offset_x + WIDGET_PADDING + global_ui_context.camera->x;
	s32 y = global_ui_context.layout.offset_y + global_ui_context.camera->y + ui_get_scroll();
	s32 text_x = x + 5;
	s32 text_y = y + (TEXTBOX_HEIGHT/2) - (global_ui_context.font_small->size/2)+2;
	s32 mouse_x = global_ui_context.mouse->x + global_ui_context.camera->x;
	s32 mouse_y = global_ui_context.mouse->y + global_ui_context.camera->y;
	
	if (global_ui_context.layout.block_height < TEXTBOX_HEIGHT)
		global_ui_context.layout.block_height = TEXTBOX_HEIGHT;
	
	bool has_text = state->buffer[0] != 0;
	
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
	
	if (mouse_x >= x && mouse_x < x + TEXTBOX_WIDTH && mouse_y >= virt_top && mouse_y < virt_bottom)
	{
		if (is_left_clicked(global_ui_context.mouse))
		{
			keyboard_set_input_text(global_ui_context.keyboard, state->buffer);
			cursor_tick = 0;
			
			state->state = !state->state;
			global_ui_context.mouse->left_state &= ~MOUSE_CLICK;
			result = true;
			
			global_ui_context.keyboard->take_input = state->state;
		}
	}
	else if (is_left_clicked(global_ui_context.mouse) || is_left_down(global_ui_context.mouse))
	{
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
		strncpy(state->buffer, global_ui_context.keyboard->input_text, state->max_len);
		
		// draw cursor
		char *calculate_text = malloc(MAX_INPUT_LENGTH);
		strcpy(calculate_text, global_ui_context.keyboard->input_text);
		calculate_text[global_ui_context.keyboard->cursor] = 0;
		
		if (last_cursor_pos != global_ui_context.keyboard->cursor)
			cursor_tick = 0;
		last_cursor_pos = global_ui_context.keyboard->cursor;
		
		cursor_text_w = calculate_text_width(global_ui_context.font_small, 
											 calculate_text);
		free(calculate_text);
		
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
		
		if (cursor_tick % 50 < 25)
			render_rectangle(cursor_x, cursor_y, cursor_w, cursor_h, global_ui_context.style.textbox_foreground);
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

bool ui_push_checkbox(checkbox_state *state, char *title)
{
	bool result = false;
	
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
	
	if (mouse_x >= x && mouse_x < x + CHECKBOX_SIZE && mouse_y >= virt_top && mouse_y < virt_bottom)
	{
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

bool ui_push_menu_item(char *title, char *shortcut)
{
	bool result = false;
	
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
		bg_color = global_ui_context.style.background_hover;
		
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

bool ui_push_button(button_state *state, char *title)
{
	bool result = false;
	
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
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom)
	{
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


bool ui_push_button_image(button_state *state, char *title, image *img)
{
	bool result = false;
	
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
	
	if (mouse_x >= x && mouse_x < x + total_w && mouse_y >= virt_top && mouse_y < virt_bottom)
	{
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
