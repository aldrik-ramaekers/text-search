/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_ASSETS
#define INCLUDE_ASSETS

#ifndef ASSET_IMAGE_COUNT
#define ASSET_IMAGE_COUNT 10
#endif

#ifndef ASSET_FONT_COUNT
#define ASSET_FONT_COUNT 10
#endif

#ifndef ASSET_QUEUE_COUNT
#define ASSET_QUEUE_COUNT 20
#endif

#ifdef MODE_DEVELOPER
s32 __frames_drawn_with_missing_assets = 0;
#endif


typedef struct t_image {
	u8 *start_addr;
	u8 *end_addr;
	bool loaded;
	s32 width;
	s32 height;
	s32 channels;
	void *data;
	s16 references;
	u32 textureID;
} image;

#define TEXT_CHARSET_START 0
#define TEXT_CHARSET_END 2000
#define TOTAL_GLYPHS TEXT_CHARSET_END-TEXT_CHARSET_START

typedef struct t_glyph
{
	s32 width;
	s32 height;
	s32 advance;
	s32 lsb;
	s32 xoff;
	s32 yoff;
	void *bitmap;
	u32 textureID;
} glyph;

typedef struct t_font
{
	u8 *start_addr;
	u8 *end_addr;
	bool loaded;
	s16 references;
	s16 size;
	s32 px_h;
	float32 scale;
	stbtt_fontinfo info;
	glyph glyphs[TOTAL_GLYPHS];
} font;

typedef enum t_asset_task_type
{
	ASSET_IMAGE,
	ASSET_BITMAP,
	ASSET_FONT,
} asset_task_type;

typedef struct t_asset_task
{
	s8 type;
	bool valid;
	union {
		image *image;
		font *font;
	};
} asset_task;

typedef struct t_asset_queue {
	array queue;
} asset_queue;

typedef struct t_assets {
	array images;
	array fonts;
	asset_queue queue;
	array post_process_queue;
	bool valid;
	bool done_loading_assets;
} assets;

char *binary_path;

mutex asset_mutex;
assets global_asset_collection;

void assets_create();
void assets_destroy();

bool assets_do_post_process();
void *assets_queue_worker();

image *assets_load_image(u8 *start_addr, u8 *end_addr);
void assets_destroy_image(image *image);

image *assets_load_bitmap(u8 *start_addr, u8 *end_addr);
void assets_destroy_bitmap(image *image);

font *assets_load_font(u8 *start_addr, u8 *end_addr, s16 size);
void assets_destroy_font(font *font);

void assets_switch_render_method();

#define load_image(_name, _inmem) assets_load_image(_binary____data_imgs_##_name##_start,_binary____data_imgs_##_name##_end)
#define load_font(_name, _size) assets_load_font(_binary____data_fonts_##_name##_start,_binary____data_fonts_##_name##_end, _size)
#define load_bitmap(_name) assets_load_bitmap(_binary____data_imgs_##_name##_start,_binary____data_imgs_##_name##_end)

#endif