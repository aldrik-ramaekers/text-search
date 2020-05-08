/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_TESTS
#define INCLUDE_TESTS

platform_window *__window; 
keyboard_input *__keyboard; 
mouse_input *__mouse;

s32 __count = 1;
void __report_result(char *text, bool success)
{
	char buf[MAX_INPUT_LENGTH];
	sprintf(buf, "#%d: success: %s - %s\n", __count++, success?"TRUE":"FALSE", text);
	printf(buf);
}

void __simulate_click()
{
	thread_sleep(100000);
	__mouse->left_state = MOUSE_CLICK;
	thread_sleep(100000);
	__mouse->left_state = MOUSE_RELEASE;
	thread_sleep(100000);
	__mouse->left_state = 0;
}

void* _run_tests()
{
	thread_sleep(1000); // wait for startup
	
	// reset /////////////////
	__mouse->left_state &= ~MOUSE_CLICK;
	__mouse->right_state &= ~MOUSE_CLICK;
	__mouse->left_state &= ~MOUSE_DOUBLE_CLICK;
	__mouse->right_state &= ~MOUSE_DOUBLE_CLICK;
	__mouse->left_state &= ~MOUSE_RELEASE;
	__mouse->right_state &= ~MOUSE_RELEASE;
	memset(__keyboard->input_keys, 0, MAX_KEYCODE);
	__mouse->move_x = 0;
	__mouse->move_y = 0;
	__mouse->scroll_state = 0;
	__keyboard->text_changed = false;
	//////////////////////////
	
	string_copyn(textbox_search_text.buffer, "#include \"stb_", MAX_INPUT_LENGTH);
	string_copyn(textbox_path.buffer, "C:\\Users\\aldri\\Desktop\\Vault\\Projects\\project-base\\src", MAX_INPUT_LENGTH);
	string_copyn(textbox_file_filter.buffer, "*.c,*.h", MAX_INPUT_LENGTH);
	checkbox_recursive.state = 1;
	
	__mouse->x = 300;
	__mouse->y = 80;
	
	__simulate_click();
	
	__mouse->x = 500;
	__mouse->y = 500;
	
	thread_sleep(100000);
	
	if (current_search_result->matches.length == 6) {
		__report_result("search files & text", true);
	}
	else {
		__report_result("search files & text", false);
	}
	
	return 0;
}

void run_tests(platform_window *window, keyboard_input *keyboard, mouse_input *mouse)
{
	__window = window;
	__keyboard = keyboard;
	__mouse = mouse;
	
	thread test_thread = thread_start(_run_tests, NULL);
	thread_detach(&test_thread);
}

#endif