
inline u8 is_left_down(mouse_input *input)
{
	return input->left_state & MOUSE_DOWN;
}

inline u8 is_left_released(mouse_input *input)
{
	return input->left_state & MOUSE_RELEASE;
}

inline u8 is_left_clicked(mouse_input *input)
{
	return input->left_state & MOUSE_CLICK;
}

inline u8 is_right_down(mouse_input *input)
{
	return input->right_state & MOUSE_DOWN;
}

inline u8 is_right_released(mouse_input *input)
{
	return input->right_state & MOUSE_RELEASE;
}

inline u8 is_right_clicked(mouse_input *input)
{
	return input->right_state & MOUSE_CLICK;
}

inline u8 keyboard_is_key_down(keyboard_input *keyboard, s16 key)
{
	return keyboard->keys[key];
}

inline u8 keyboard_is_key_pressed(keyboard_input *keyboard, s16 key)
{
	return keyboard->input_keys[key];
}

inline void keyboard_set_input_text(keyboard_input *keyboard, char *text)
{
	strncpy(keyboard->input_text, text, MAX_INPUT_LENGTH);
	u32 len = strlen(keyboard->input_text);
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

void keyboard_handle_input_string(keyboard_input *keyboard, char *ch)
{
	if (ch)
	{
		u8 is_lctrl_down = keyboard->keys[KEY_LEFT_CONTROL];
		
		if (keyboard->input_text_len < MAX_INPUT_LENGTH && !is_lctrl_down)
		{
			if (keyboard->input_text_len)
			{
				char buffer[MAX_INPUT_LENGTH];
				if (keyboard->cursor)
				{
					char ch2 = keyboard->input_text[keyboard->cursor-1];
					keyboard->input_text[keyboard->cursor-1] = 0;
					
					sprintf(buffer, "%s%c%c%s", keyboard->input_text, ch2, *ch, keyboard->input_text+keyboard->cursor);
				}
				else
				{
					sprintf(buffer, "%c%s", *ch, keyboard->input_text);
				}
				
				strcpy(keyboard->input_text, buffer);
			}
			else
			{
				strcat(keyboard->input_text, ch);
			}
			
			keyboard->cursor++;
			keyboard->input_text_len++;
		}
	}
	else
	{
		u8 is_lctrl_down = keyboard->keys[KEY_LEFT_CONTROL];
		
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
				keyboard->cursor = keyboard->input_text_len;
			else
				keyboard->cursor++;
		}
	}
	
	if (keyboard_is_key_down(keyboard, KEY_BACKSPACE))
	{
		u8 is_lctrl_down = keyboard->keys[KEY_LEFT_CONTROL];
		
		if (is_lctrl_down)
		{
			keyboard->input_text[0] = '\0';
			strcpy(keyboard->input_text, keyboard->input_text+keyboard->cursor);
			keyboard->input_text_len -= keyboard->cursor;
			keyboard->cursor = 0;
		}
		else if (keyboard->cursor > 0)
		{
			s32 len = strlen(keyboard->input_text);
			//keyboard->input_text[len-1] = '\0';
			strcpy(keyboard->input_text+keyboard->cursor-1, keyboard->input_text+keyboard->cursor);
			
			keyboard->cursor--;
			keyboard->input_text_len--;
		}
	}
}