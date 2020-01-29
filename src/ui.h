/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_UI
#define INCLUDE_UI

#define BLOCK_HEIGHT 25
#define MENU_BAR_HEIGHT 25
#define MENU_HORIZONTAL_PADDING 10
#define WIDGET_PADDING 8
#define BUTTON_HORIZONTAL_TEXT_PADDING 15
#define MENU_ITEM_WIDTH 190
#define CHECKBOX_SIZE BLOCK_HEIGHT - 8
#define TEXTBOX_WIDTH 300
#define TEXTBOX_HEIGHT BLOCK_HEIGHT
#define BUTTON_HEIGHT BLOCK_HEIGHT
#define BUTTON_IMAGE_PADDING 5
#define BUTTON_IMAGE_SPACING 8
#define DROPDOWN_WIDTH 225
#define DROPDOWN_ITEM_WIDTH 225
#define TEXTBOX_SCROLL_X_SPEED 32

typedef enum t_ui_style_type
{
	UI_STYLE_LIGHT = 1,
	UI_STYLE_DARK = 2,
} ui_style_type;

typedef struct t_ui_style
{
	u16 id;
	color foreground;
	color background;
	color border;
	color textbox_background;
	color textbox_active_border;
	color textbox_foreground;
	color image_outline_tint;
	color scrollbar_handle_background;
	color info_bar_background;
	color error_foreground;
	color item_hover_background;
	color scrollbar_background;
	color menu_background;
	color menu_hover_background;
	color menu_foreground;
	color widget_hover_background;
	color widget_background;
	color hypertext_foreground;
	color hypertext_hover_foreground;
	color textbox_placeholder_foreground;
} ui_style;

typedef enum t_layout_direction
{
	LAYOUT_HORIZONTAL,
	LAYOUT_VERTICAL,
} layout_direction;

typedef struct t_scroll_state
{
	s32 height;
	s32 scroll;
	s32 scroll_start_offset_y;
	bool in_scroll;
} scroll_state;

typedef struct t_ui_layout
{
	s32 dropdown_item_count;
	s32 dropdown_x;
	s32 offset_x;
	s32 offset_y;
	platform_window *active_window;
	layout_direction layout_direction;
	s32 prev_offset_x;
	s32 width;
	s32 menu_offset_y;
	s32 block_height;
	s32 start_offset_y;
	s32 start_offset_x;
	scroll_state scroll;
	s32 padding;
} ui_layout;

typedef struct t_textbox_history_entry
{
	char *text;
	s32 cursor_offset;
} textbox_history_entry;

typedef struct t_textbox_state
{
	bool deselect_on_enter;
	char *buffer;
	s32 selection_start_index;
	bool state;
	s32 diff;
	bool double_clicked_to_select;
	s32 double_clicked_to_select_cursor_index;
	s32 max_len;
	s32 text_offset_x;
	bool attempting_to_select;
	array history;
	array future;
	s32 last_click_cursor_index;
} textbox_state;

typedef struct t_checkbox_state
{
	bool state;
} checkbox_state;

typedef struct t_button_state
{
	bool state;
} button_state;

typedef struct t_dropdown_state
{
	bool state;
} dropdown_state;

typedef struct t_ui_context
{
	ui_style style;
	ui_layout layout;
	keyboard_input *keyboard;
	mouse_input *mouse;
	camera *camera;
	font *font_small;
	array active_menus;
	u32 next_id;
	s32 menu_item_count;
	dropdown_state *active_dropdown;
	textbox_state *current_active_textbox;
	bool item_hovered;
} ui_context;

///// our global ui states ////
checkbox_state checkbox_recursive;
textbox_state textbox_search_text;
textbox_state textbox_path;
textbox_state textbox_file_filter;
button_state button_select_directory;
button_state button_find_text;
button_state button_cancel;
///////////////////////////////

ui_context global_ui_context;

u32 ui_get_id();
void ui_create(platform_window *window, keyboard_input *keyboard, mouse_input *mouse, camera *camera, font *font_small);
void ui_set_active_window(platform_window *window);
void ui_destroy();
void ui_begin();
void ui_end();
bool ui_is_menu_active(u32 id);
char* name_of_day(s32 day);
char* name_of_month(s32 month);
void ui_set_style(u16 style);
void set_active_textbox(textbox_state *textbox);

// widget initialization
checkbox_state ui_create_checkbox(bool selected);
textbox_state ui_create_textbox(u16 max_len);
button_state ui_create_button();
scroll_state ui_create_scroll(s32 scroll);
dropdown_state ui_create_dropdown();

void ui_destroy_textbox(textbox_state *state);

// widgets
bool is_shortcut_down(s32 shortcut_keys[2]);
void ui_begin_menu_bar();
bool ui_push_menu(char *title);
bool ui_push_menu_item(char *title, char *shortcut);
void ui_push_menu_item_separator();
bool ui_push_dropdown(dropdown_state *state, char *title);
bool ui_push_dropdown_item(image *icon, char *title);
void ui_push_separator();
void ui_block_begin(layout_direction direction);
void ui_block_end();
void ui_end_menu_bar();
void ui_push_text(char *text);
bool ui_push_hypertext_link(char *text);
bool ui_push_color_button(char *text, bool selected, color color);
bool ui_push_image(image *img, s32 w, s32 h, s32 outline, color tint);
bool ui_push_checkbox(checkbox_state *state, char *title);
bool ui_push_textbox(textbox_state *state, char *title);
bool ui_push_button(button_state *button, char *title);
bool ui_push_button_image(button_state *button, char *title, image *img);
void ui_scroll_begin(s32 height);
void ui_scroll_end();

#endif