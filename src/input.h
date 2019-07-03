#ifndef INCLUDE_INPUT
#define INCLUDE_INPUT

/* The unknown key */
#define KEY_UNKNOWN -1

#define MOUSE_OFFSCREEN 32767

/* Printable keys */
#define KEY_SPACE              32
#define KEY_APOSTROPHE         39  /* ' */
#define KEY_COMMA              44  /* , */
#define KEY_MINUS              45  /* - */
#define KEY_PERIOD             46  /* . */
#define KEY_SLASH              47  /* / */
#define KEY_0                  48
#define KEY_1                  49
#define KEY_2                  50
#define KEY_3                  51
#define KEY_4                  52
#define KEY_5                  53
#define KEY_6                  54
#define KEY_7                  55
#define KEY_8                  56
#define KEY_9                  57
#define KEY_SEMICOLON          59  /* ; */
#define KEY_EQUAL              61  /* = */
#define KEY_A                  65
#define KEY_B                  66
#define KEY_C                  67
#define KEY_D                  68
#define KEY_E                  69
#define KEY_F                  70
#define KEY_G                  71
#define KEY_H                  72
#define KEY_I                  73
#define KEY_J                  74
#define KEY_K                  75
#define KEY_L                  76
#define KEY_M                  77
#define KEY_N                  78
#define KEY_O                  79
#define KEY_P                  80
#define KEY_Q                  81
#define KEY_R                  82
#define KEY_S                  83
#define KEY_T                  84
#define KEY_U                  85
#define KEY_V                  86
#define KEY_W                  87
#define KEY_X                  88
#define KEY_Y                  89
#define KEY_Z                  90
#define KEY_LEFT_BRACKET       91  /* [ */
#define KEY_BACKSLASH          92  /* \ */
#define KEY_RIGHT_BRACKET      93  /* ] */
#define KEY_GRAVE_ACCENT       96  /* ` */
#define KEY_WORLD_1            161 /* non-US #1 */
#define KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define KEY_ESCAPE             256
#define KEY_ENTER              257
#define KEY_TAB                258
#define KEY_BACKSPACE          259
#define KEY_INSERT             260
#define KEY_DELETE             261
#define KEY_RIGHT              262
#define KEY_LEFT               263
#define KEY_DOWN               264
#define KEY_UP                 265
#define KEY_PAGE_UP            266
#define KEY_PAGE_DOWN          267
#define KEY_HOME               268
#define KEY_END                269
#define KEY_CAPS_LOCK          280
#define KEY_SCROLL_LOCK        281
#define KEY_NUM_LOCK           282
#define KEY_PRINT_SCREEN       283
#define KEY_PAUSE              284
#define KEY_F1                 290
#define KEY_F2                 291
#define KEY_F3                 292
#define KEY_F4                 293
#define KEY_F5                 294
#define KEY_F6                 295
#define KEY_F7                 296
#define KEY_F8                 297
#define KEY_F9                 298
#define KEY_F10                299
#define KEY_F11                300
#define KEY_F12                301
#define KEY_F13                302
#define KEY_F14                303
#define KEY_F15                304
#define KEY_F16                305
#define KEY_F17                306
#define KEY_F18                307
#define KEY_F19                308
#define KEY_F20                309
#define KEY_F21                310
#define KEY_F22                311
#define KEY_F23                312
#define KEY_F24                313
#define KEY_F25                314
#define KEY_KP_0               320
#define KEY_KP_1               321
#define KEY_KP_2               322
#define KEY_KP_3               323
#define KEY_KP_4               324
#define KEY_KP_5               325
#define KEY_KP_6               326
#define KEY_KP_7               327
#define KEY_KP_8               328
#define KEY_KP_9               329
#define KEY_KP_DECIMAL         330
#define KEY_KP_DIVIDE          331
#define KEY_KP_MULTIPLY        332
#define KEY_KP_SUBTRACT        333
#define KEY_KP_ADD             334
#define KEY_KP_ENTER           335
#define KEY_KP_EQUAL           336
#define KEY_LEFT_SHIFT         340
#define KEY_LEFT_CONTROL       341
#define KEY_LEFT_ALT           342
#define KEY_LEFT_SUPER         343
#define KEY_RIGHT_SHIFT        344
#define KEY_RIGHT_CONTROL      345
#define KEY_RIGHT_ALT          346
#define KEY_RIGHT_SUPER        347
#define KEY_MENU               348

#define KEY_LAST KEY_MENU

#define MAX_KEYCODE 512

#define MOUSE_DOWN (1 << 1)
#define MOUSE_RELEASE (1 << 2)
#define MOUSE_DOUBLE_CLICK (1 << 3)
#define MOUSE_CLICK (1 << 4)

#define SCROLL_UP 1
#define SCROLL_DOWN -1


// should be max path length
#ifdef OS_LINUX
#define MAX_INPUT_LENGTH 4096+1
#define MAX_PATH_LENGTH 255+1
#endif

#ifdef OS_WINDOWS
#define MAX_INPUT_LENGTH 4096+1
#define MAX_PATH_LENGTH 259+1
#endif

typedef struct t_mouse_input
{
	s16 x;
	s16 y;
	s16 move_x;
	s16 move_y;
	s8 left_state;
	s8 right_state;
	s8 scroll_state;
} mouse_input;

typedef enum t_keyboard_input_mode
{
	INPUT_NUMERIC,
	INPUT_FULL,
} keyboard_input_mode;

typedef struct t_keyboard_input
{
	keyboard_input_mode input_mode;
	int modifier_state;
	bool take_input;
	u32 cursor;
	char *input_text;
	s32 input_text_len;
	bool keys[MAX_KEYCODE];
	bool input_keys[MAX_KEYCODE];
} keyboard_input;

int keycode_map[MAX_KEYCODE];

u8 is_left_down(mouse_input *input);
u8 is_left_released(mouse_input *input);
u8 is_left_clicked(mouse_input *input);
u8 is_left_double_clicked(mouse_input *input);
u8 is_right_down(mouse_input *input);
u8 is_right_released(mouse_input *input);
u8 is_right_clicked(mouse_input *input);

u8 keyboard_is_key_down(keyboard_input *keyboard, s16 key);
u8 keyboard_is_key_pressed(keyboard_input *keyboard, s16 key);
void keyboard_set_input_text(keyboard_input *keyboard, char *text);
void keyboard_set_input_mode(keyboard_input *keyboard, keyboard_input_mode mode);

typedef struct t_platform_window platform_window;
void keyboard_handle_input_string(platform_window *window, keyboard_input *keyboard, char *text);

mouse_input mouse_input_create();
keyboard_input keyboard_input_create();
void keyboard_input_destroy(keyboard_input *keyboard);

#endif