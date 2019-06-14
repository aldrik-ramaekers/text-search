#ifndef INCLUDE_UI
#define INCLUDE_UI

#define BLOCK_HEIGHT 25
#define MENU_BAR_HEIGHT 25
#define MENU_HORIZONTAL_PADDING 10
#define WIDGET_PADDING 8
#define BUTTON_HORIZONTAL_TEXT_PADDING 15
#define MENU_ITEM_WIDTH 170
#define CHECKBOX_SIZE BLOCK_HEIGHT - 8
#define TEXTBOX_WIDTH 270
#define TEXTBOX_HEIGHT BLOCK_HEIGHT
#define BUTTON_HEIGHT BLOCK_HEIGHT
#define BUTTON_IMAGE_PADDING 5
#define BUTTON_IMAGE_SPACING 8

typedef struct t_ui_style
{
	color foreground;
	color background;
	color background_hover;
	color border;
	color textbox_background;
	color textbox_active_border;
	color textbox_foreground;
	color button_background;
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

typedef struct t_textbox_state
{
	char *buffer;
	bool state;
	s32 selection_start_cursor;
	s32 selection_length;
	s32 max_len;
	s32 text_offset_x;
} textbox_state;

typedef struct t_checkbox_state
{
	bool state;
} checkbox_state;

typedef struct t_button_state
{
	bool state;
} button_state;

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
} ui_context;


ui_context global_ui_context;

u32 ui_get_id();
void ui_create(platform_window *window, keyboard_input *keyboard, mouse_input *mouse, camera *camera, font *font_small);
void ui_set_active_window(platform_window *window);
void ui_destroy();
void ui_begin();
void ui_end();
bool ui_is_menu_active(u32 id);

// widget initialization
checkbox_state ui_create_checkbox(bool selected);
textbox_state ui_create_textbox(u16 max_len);
button_state ui_create_button();
scroll_state ui_create_scroll(s32 scroll);

void ui_destroy_textbox(textbox_state *state);

// widgets
bool is_shortcut_down(s32 shortcut_keys[2]);
void ui_begin_menu_bar();
bool ui_push_menu(char *title);
bool ui_push_menu_item(char *title, char *shortcut);
void ui_push_menu_item_separator();
void ui_push_separator();
void ui_block_begin(layout_direction direction);
void ui_block_end();
void ui_end_menu_bar();
void ui_push_text(char *text);
bool ui_push_checkbox(checkbox_state *state, char *title);
bool ui_push_textbox(textbox_state *state, char *title);
bool ui_push_button(button_state *button, char *title);
bool ui_push_button_image(button_state *button, char *title, image *img);
void ui_scroll_begin(s32 height);
void ui_scroll_end();

#endif