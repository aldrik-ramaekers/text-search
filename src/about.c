
void about_page_create()
{
	global_about_page.active = false;
	global_about_page.sloth_img = assets_load_image("data/imgs/sloth.png", false);
	global_about_page.sloth_small_img = assets_load_image("data/imgs/sloth_small.png", true);
	global_about_page.font_big = assets_load_font("data/fonts/mono.ttf", 32);
	global_about_page.font_small = assets_load_font("data/fonts/mono.ttf", 16);
	global_about_page.keyboard = keyboard_input_create();
	global_about_page.mouse = mouse_input_create();
	
	global_about_page.btn_close = ui_create_button();
	global_about_page.btn_website = ui_create_button();
}

void about_page_update_render()
{
	if (global_about_page.active)
	{
		platform_window_make_current(&global_about_page.window);
		platform_handle_events(&global_about_page.window, &global_about_page.mouse, &global_about_page.keyboard);
		
		glClearColor(255/255.0, 255/255.0, 255/255.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		s32 text_x = 20;
		s32 text_y = 20;
		s32 img_w  = 50;
		s32 img_h  = 50;
		s32 img_y  = text_y - (img_h/2) + (global_about_page.font_big->size/2);
		
		char *title_text = "Text-search";
		
		time_t timeval;
		struct tm *tp;
		time (&timeval);
		tp = gmtime(&timeval);
		
		char copyright_text[30];
		sprintf(copyright_text, "Copyright 2019-%d", tp->tm_year+1900);
		
		char *info_text = localize("short_info_text");
		char *names_text = "Aldrik Ramaekers";
		
		s32 text_w = calculate_text_width(global_about_page.font_big, title_text);
		text_x = -(text_w/2)+(global_about_page.window.width/2);
		render_text(global_about_page.font_big, 
					text_x, text_y, title_text, rgb(20,20,20));
		
		render_image(global_about_page.sloth_img, text_x + text_w + 30, img_y, img_w, img_h);
		
		// info
		text_y += global_about_page.font_big->size + 30;
		text_x = -(calculate_text_width(global_about_page.font_small, info_text)/2)
			+ (global_about_page.window.width/2);
		render_text(global_about_page.font_small, text_x, text_y,
					info_text, rgb(20,20,20));
		
		// copyright
		text_y += global_about_page.font_small->size + 20;
		text_x = -(calculate_text_width(global_about_page.font_small, copyright_text)/2)
			+ (global_about_page.window.width/2);
		render_text(global_about_page.font_small, text_x, text_y,
					copyright_text, rgb(20,20,20));
		
		// names
		text_y += global_about_page.font_small->size;
		text_x = -(calculate_text_width(global_about_page.font_small, names_text)/2)
			+ (global_about_page.window.width/2);
		render_text(global_about_page.font_small, text_x, text_y, 
					names_text, rgb(20,20,20));
		
		global_ui_context.layout.active_window = &global_about_page.window;
		global_ui_context.keyboard = &global_about_page.keyboard;
		global_ui_context.mouse = &global_about_page.mouse;
		
		ui_begin(2);
		{
			global_ui_context.layout.offset_y = global_about_page.window.height - 33;
			
			if (ui_push_button(&global_about_page.btn_close, localize("close")))
			{
				global_about_page.active = false;
				about_page_hide();
				return;
			}
			if (ui_push_button(&global_about_page.btn_website, localize("website")))
			{
				platform_open_url(WEBSITE_URL);
			}
		}
		ui_end();
		
		if (!global_about_page.window.is_open)
		{
			global_about_page.active = false;
			about_page_hide();
			return;
		}
		
		platform_window_swap_buffers(&global_about_page.window);
	}
}

void about_page_show()
{
	if (platform_window_is_valid(&global_about_page.window)) return;
	
	global_about_page.window = platform_open_window(localize("about_text_search"), 450, 250, 450, 450);
	global_about_page.active = true;
	platform_set_icon(&global_about_page.window, global_about_page.sloth_small_img);
}

void about_page_hide()
{
	if (platform_window_is_valid(&global_about_page.window))
	{
		platform_close_window(&global_about_page.window);
		
		global_about_page.btn_close.state = false;
		global_about_page.btn_website.state = false;
		global_about_page.active = false;
		
		global_about_page.mouse.x = -1;
		global_about_page.mouse.y = -1;
	}
}

void about_page_destroy()
{
	keyboard_input_destroy(&global_about_page.keyboard);
	assets_destroy_image(global_about_page.sloth_img);
	assets_destroy_image(global_about_page.sloth_small_img);
	assets_destroy_font(global_about_page.font_big);
	assets_destroy_font(global_about_page.font_small);
}
