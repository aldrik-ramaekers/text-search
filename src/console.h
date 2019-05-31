#ifndef INCLUDE_CONSOLE
#define INCLUDE_CONSOLE

#ifndef CONSOLE_MESSAGE_COLOR
#define CONSOLE_MESSAGE_COLOR rgb(250,250,250)
#endif

#ifndef CONSOLE_ERROR_COLOR
#define CONSOLE_ERROR_COLOR rgb(255,19,19)
#endif

typedef enum t_console_print_type
{
	PRINT_MESSAGE,
	PRINT_ERROR,
} console_print_type;

typedef struct t_console_message
{
	char *message;
	console_print_type type;
} console_message;

typedef struct t_console
{
	array log;
} console;

mutex console_mutex;

void console_create();
void console_update_render(platform_window *window, camera *camera, mouse_input *mouse, s32 x, s32 y, s32 w, s32 h);
void console_print(char *message
				   ,console_print_type type, ...) __attribute__ ((format (printf, 1, 3)));
void console_destroy();

#endif