
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

inline void keyboard_set_input_text(keyboard_input *keyboard, char *text)
{
	strcpy(keyboard->input_text, text);
	u32 len = strlen(keyboard->input_text);
	keyboard->cursor = len;
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
	keyboard.input_text = malloc(2500);
	keyboard.input_text[0] = '\0';
	keyboard.take_input = false;
	keyboard.cursor = 0;
	memset(keyboard.keys, 0, MAX_KEYCODE);
	
	return keyboard;
}

inline void keyboard_input_destroy(keyboard_input *keyboard)
{
	free(keyboard->input_text);
}