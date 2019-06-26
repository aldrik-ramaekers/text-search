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
	button_state btn_save;
	dropdown_state dropdown_language;
	textbox_state textbox_max_file_size;
	textbox_state textbox_max_thread_count;
	checkbox_state checkbox_parallelize_search;
	char *current_locale_id;
	
	s32 selected_tab_index;
	s32 max_thread_count;
	s32 max_file_size;
	u8 enable_parallelization;
} settings_page;

settings_page global_settings_page;

void settings_page_create();
void settings_page_hide_without_save();
void settings_page_update_render();
void settings_page_show();
void settings_page_hide();
void settings_page_destroy();

#endif