/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.

*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.

*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef INCLUDE_ASSETS
#define INCLUDE_ASSETS

#ifndef ASSET_IMAGE_COUNT
#define ASSET_IMAGE_COUNT 40
#endif

#ifndef ASSET_FONT_COUNT
#define ASSET_FONT_COUNT 20
#endif

#ifndef ASSET_QUEUE_COUNT
#define ASSET_QUEUE_COUNT 100
#endif

// binary blobs
extern u8 _binary____data_imgs_en_png_start[];
extern u8 _binary____data_imgs_en_png_end[];

extern u8 _binary____data_imgs_error_png_start[];
extern u8 _binary____data_imgs_error_png_end[];

extern u8 _binary____data_imgs_folder_png_start[];
extern u8 _binary____data_imgs_folder_png_end[];

extern u8 _binary____data_imgs_nl_png_start[];
extern u8 _binary____data_imgs_nl_png_end[];

extern u8 _binary____data_imgs_search_png_start[];
extern u8 _binary____data_imgs_search_png_end[];

extern u8 _binary____data_imgs_logo_64_png_start[];
extern u8 _binary____data_imgs_logo_64_png_end[];

extern u8 _binary____data_imgs_logo_512_png_start[];
extern u8 _binary____data_imgs_logo_512_png_end[];

extern u8 _binary____data_fonts_mono_ttf_start[];
extern u8 _binary____data_fonts_mono_ttf_end[];

extern u8 _binary____data_translations_en_English_mo_start[];
extern u8 _binary____data_translations_en_English_mo_end[];

extern u8 _binary____data_translations_nl_Dutch_mo_start[];
extern u8 _binary____data_translations_nl_Dutch_mo_end[];

typedef struct t_image {
	u8 *start_addr;
	u8 *end_addr;
	bool loaded;
	bool keep_in_memory;
	s32 width;
	s32 height;
	s32 channels;
	void *data;
	s16 references;
	GLuint textureID;
} image;

#define TEXT_CHARSET_START 0
#define GLYPHS_PER_BITMAP 512
#define TEXT_CHARSET_END GLYPHS_PER_BITMAP*26
#define TOTAL_GLYPH_BITMAPS TEXT_CHARSET_END/GLYPHS_PER_BITMAP

typedef struct t_font
{
	u8 *start_addr;
	u8 *end_addr;
	bool loaded;
	s16 references;
	s16 size;
	GLuint textureIDs[TOTAL_GLYPH_BITMAPS];
	s32 palette_width;
	s32 palette_height;
	s16 glyph_widths[TOTAL_GLYPH_BITMAPS*TEXT_CHARSET_END+1];
	float32 scale;
	stbtt_fontinfo info;
	void *bitmaps[TOTAL_GLYPH_BITMAPS];
} font;

typedef enum t_asset_task_type
{
	ASSET_IMAGE,
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
} assets;

char *binary_path;

mutex asset_mutex;
assets global_asset_collection;

void assets_create();
void assets_destroy();

void assets_do_post_process();
void *assets_queue_worker();

image *assets_load_image(u8 *start_addr, u8 *end_addr, bool keep_in_memory);
void assets_destroy_image(image *image);

font *assets_load_font(u8 *start_addr, u8 *end_addr, s16 size);
void assets_destroy_font(font *font);

#endif