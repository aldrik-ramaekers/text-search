/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

inline bool is_left_down(mouse_input *input)
{
	return input->left_state & MOUSE_DOWN;
}

inline bool is_left_released(mouse_input *input)
{
	return input->left_state & MOUSE_RELEASE;
}

inline bool is_left_clicked(mouse_input *input)
{
	return input->left_state & MOUSE_CLICK;
}

inline bool is_left_double_clicked(mouse_input *input)
{
	return input->left_state & MOUSE_DOUBLE_CLICK;
}

inline bool is_right_down(mouse_input *input)
{
	return input->right_state & MOUSE_DOWN;
}

inline bool is_right_released(mouse_input *input)
{
	return input->right_state & MOUSE_RELEASE;
}

inline bool is_right_clicked(mouse_input *input)
{
	return input->right_state & MOUSE_CLICK;
}

inline bool keyboard_is_key_down(keyboard_input *keyboard, s16 key)
{
	return keyboard->keys[key];
}

inline bool keyboard_is_key_pressed(keyboard_input *keyboard, s16 key)
{
	return keyboard->input_keys[key];
}

inline void keyboard_set_input_text(keyboard_input *keyboard, char *text)
{
	string_copyn(keyboard->input_text, text, MAX_INPUT_LENGTH);
	u32 len = utf8len(keyboard->input_text);
	keyboard->cursor = len;
	keyboard->input_text_len = len;
}

mouse_input mouse_input_create()
{
	mouse_input mouse;
	mouse.x = -1;
	mouse.y = -1;
	mouse.move_x = 0;
	mouse.move_y = 0;
	mouse.left_state = 0;
	mouse.right_state = 0;
	
	return mouse;
}

keyboard_input keyboard_input_create()
{
	keyboard_input keyboard;
	keyboard.modifier_state = 0;
	keyboard.input_text = mem_alloc(MAX_INPUT_LENGTH);
	keyboard.input_text[0] = '\0';
	keyboard.take_input = false;
	keyboard.cursor = 0;
	keyboard.input_text_len = 0;
	keyboard.input_mode = INPUT_FULL;
	keyboard.has_selection = false;
	keyboard.selection_begin_offset = 0;
	keyboard.selection_length = 0;
	memset(keyboard.keys, 0, MAX_KEYCODE);
	
	return keyboard;
}

void keyboard_set_input_mode(keyboard_input *keyboard, keyboard_input_mode mode)
{
	keyboard->input_mode = mode;
}

inline void keyboard_input_destroy(keyboard_input *keyboard)
{
	mem_free(keyboard->input_text);
}

inline static void keyboard_handle_input_copy_and_paste(platform_window *window, keyboard_input *keyboard)
{
	bool is_lctrl_down = keyboard->keys[KEY_LEFT_CONTROL];
	
	if (is_lctrl_down && keyboard_is_key_pressed(keyboard, KEY_V))
	{
		char buf[MAX_INPUT_LENGTH];
		bool result = platform_get_clipboard(window, buf);
		
		if (keyboard->has_selection)
		{
			keyboard->cursor = keyboard->selection_begin_offset;
			utf8_str_remove_range(keyboard->input_text, keyboard->selection_begin_offset,
								  keyboard->selection_begin_offset + keyboard->selection_length);
			keyboard->has_selection = false;
			keyboard->text_changed = true;
		}
		
		if (result)
		{
			s32 len = utf8len(buf);
			utf8_str_insert_utf8str(keyboard->input_text, keyboard->cursor, buf);
			
			keyboard->cursor += len;
			keyboard->input_text_len += len;
			keyboard->text_changed = true;
		}
	}
	else if (is_lctrl_down && keyboard_is_key_pressed(keyboard, KEY_C))
	{
		char buffer[MAX_INPUT_LENGTH];
		utf8_str_copy_range(keyboard->input_text, 
							keyboard->selection_begin_offset,
							keyboard->selection_begin_offset+keyboard->selection_length,
							buffer);
		
		if (!string_equals(buffer, ""))
			platform_set_clipboard(window, buffer);
	}
}

void keyboard_handle_input_string(platform_window *window, keyboard_input *keyboard, char *ch)
{
	keyboard_handle_input_copy_and_paste(window, keyboard);
	
	bool is_lctrl_down = keyboard->keys[KEY_LEFT_CONTROL];
	
	if (ch)
	{
		if (keyboard->input_text_len < MAX_INPUT_LENGTH && !is_lctrl_down)
		{
			if (keyboard->has_selection)
			{
				keyboard->cursor = keyboard->selection_begin_offset;
				utf8_str_remove_range(keyboard->input_text, keyboard->selection_begin_offset,
									  keyboard->selection_begin_offset + keyboard->selection_length);
				keyboard->has_selection = false;
				keyboard->text_changed = true;
			}
			
			if (keyboard->input_text_len)
			{
				utf8_str_insert_at(keyboard->input_text, keyboard->cursor, *ch);
				keyboard->text_changed = true;
			}
			else
			{
				string_appendn(keyboard->input_text, ch, MAX_INPUT_LENGTH);
				keyboard->text_changed = true;
			}
			
			keyboard->cursor++;
			keyboard->input_text_len = utf8len(keyboard->input_text);
		}
	}
	else
	{
		bool is_lctrl_down = keyboard->keys[KEY_LEFT_CONTROL];
		
		// cursor movement
		if (keyboard_is_key_down(keyboard, KEY_LEFT) && keyboard->cursor > 0)
		{
			if (is_lctrl_down)
				keyboard->cursor = 0;
			else
				keyboard->cursor--;
		}
		if (keyboard_is_key_down(keyboard, KEY_RIGHT) && keyboard->cursor < keyboard->input_text_len)
		{
			if (is_lctrl_down)
				keyboard->cursor = utf8len(keyboard->input_text);
			else
				keyboard->cursor++;
		}
	}
	
	if (keyboard_is_key_down(keyboard, KEY_BACKSPACE))
	{
		bool is_lctrl_down = keyboard->keys[KEY_LEFT_CONTROL];
		
		if (keyboard->has_selection)
		{
			utf8_str_remove_range(keyboard->input_text, keyboard->selection_begin_offset,
								  keyboard->selection_begin_offset + keyboard->selection_length);
			keyboard->has_selection = false;
			keyboard->text_changed = true;
			
			if (keyboard->selection_begin_offset < keyboard->cursor)
			{
				keyboard->cursor -= keyboard->selection_length-1;
			}
		}
		else if (is_lctrl_down)
		{
			for (s32 i = 0; i < keyboard->cursor; i++)
			{
				utf8_str_remove_at(keyboard->input_text, 0);
			}
			keyboard->cursor = 0;
			keyboard->text_changed = true;
		}
		else if (keyboard->cursor > 0)
		{
			utf8_str_remove_at(keyboard->input_text, keyboard->cursor-1);
			
			keyboard->cursor--;
			keyboard->text_changed = true;
		}
		
		keyboard->input_text_len = utf8len(keyboard->input_text);
	}
}