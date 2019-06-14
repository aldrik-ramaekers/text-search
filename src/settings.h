#ifndef INCLUDE_SETTINGS
#define INCLUDE_SETTINGS

typedef struct t_settings_page
{
	platform_window window;
	keyboard_input keyboard;
	mouse_input mouse;
	bool active;
	
	font *font_small;
	image *sloth_small_img;
	
	button_state btn_close;
	textbox_state textbox_max_file_size;
	textbox_state textbox_max_thread_count;
	
	s32 selected_tab_index;
	s32 max_thread_count;
	s32 max_file_size;
	bool max_file_size_enabled;
} settings_page;

settings_page global_settings_page;

void settings_page_create();
void settings_page_update_render();
void settings_page_show();
void settings_page_hide();
void settings_page_destroy();

#endif