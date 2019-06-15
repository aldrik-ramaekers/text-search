#ifndef INCLUDE_ABOUT
#define INCLUDE_ABOUT

#include <time.h>

typedef struct t_about_page
{
	platform_window window;
	keyboard_input keyboard;
	mouse_input mouse;
	bool active;
	image *sloth_img;
	image *sloth_small_img;
	font *font_big;
	font *font_small;
	button_state btn_close;
	button_state btn_website;
} about_page;

about_page global_about_page;

void about_page_create();
void about_page_update_render();
void about_page_show();
void about_page_hide();
void about_page_destroy();

#endif