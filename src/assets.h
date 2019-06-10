#ifndef INCLUDE_ASSETS
#define INCLUDE_ASSETS

#ifndef ASSET_IMAGE_COUNT
#define ASSET_IMAGE_COUNT 40
#endif

#ifndef ASSET_FONT_COUNT
#define ASSET_FONT_COUNT 20
#endif

#ifndef ASSET_SAMPLE_COUNT
#define ASSET_SAMPLE_COUNT 40
#endif

#ifndef ASSET_QUEUE_COUNT
#define ASSET_QUEUE_COUNT 100
#endif

typedef struct t_sample
{
	char *path;
	bool loaded;
	
	s32 channels;
	s32 sample_rate;
	s32 sample_count;
	s16 *data;
	s16 references;
	float32 duration_seconds;
} sample;

typedef struct t_image {
	char *path;
	bool loaded;
	s32 width;
	s32 height;
	s32 channels;
	void *data;
	s16 references;
	GLuint textureID;
} image;

typedef struct t_font
{
	char *path;
	bool loaded;
	s16 references;
	s16 size;
	GLuint textureID;
	s32 palette_width;
	s32 palette_height;
	s16 glyph_widths[223];
	float32 scale;
	stbtt_fontinfo info;
	void *bitmap;
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
	array samples;
	asset_queue queue;
	array post_process_queue;
	bool valid;
} assets;

char *binary_path;

// TODO(Aldrik): remove asset from load queue and post process queue when destroyed

// TODO(Aldrik): we are destroying assets right now by comparing the names of every asset first while we have the pointer within the array..

// TODO(Aldrik): destroying assets is not thread safe..

// TODO(Aldrik): fix relative paths for asset loading (not sure if this is actually a problem)

mutex asset_mutex;
assets global_asset_collection;

void assets_create();
void assets_destroy();

void assets_do_post_process();
void *assets_queue_worker();

image *assets_load_image(char *file);
void assets_destroy_image(image *image);

font *assets_load_font(char *file, s16 size);
void assets_destroy_font(font *font);

sample *assets_load_sample(char *file);
void assets_destroy_sample(sample *sample);

#endif