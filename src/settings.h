/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_SETTINGS
#define INCLUDE_SETTINGS

typedef enum t_double_click_option
{
	OPTION_PATH,
	OPTION_PATH_LINE,
	OPTION_PATH_LINE_FILTER,
	OPTION_RESULT,
} double_click_option;

typedef struct t_settings_page
{
	platform_window window;
	keyboard_input keyboard;
	mouse_input mouse;
    camera camera;
	bool active;
	
	font *font_small;
	image *logo_img;
	
	button_state btn_close;
	button_state btn_save;
	dropdown_state dropdown_language;
	dropdown_state dropdown_doubleclick;
	textbox_state textbox_max_file_size;
	textbox_state textbox_max_thread_count;
	checkbox_state checkbox_parallelize_search;
	s32 selected_tab_index;
	
	char *current_locale_id;
	s32 max_thread_count;
	s32 max_file_size;
	u16 current_style;
	u16 selected_double_click_selection_option; // saved state
	u16 current_double_click_selection_option; // unsaved state 
} settings_page;

#define DEFAULT_THREAD_COUNT 10
#define DEFAULT_MAX_FILE_SIZE 0
#define DEFAULT_RECURSIVE_STATE 1
#define DEFAULT_STYLE 1

settings_page global_settings_page;

void settings_page_create();
void settings_page_hide_without_save();
void settings_page_update_render();
void settings_page_show();
void settings_page_hide();
void settings_page_destroy();

#endif