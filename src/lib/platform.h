/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_PLATFORM
#define INCLUDE_PLATFORM

typedef struct t_platform_window platform_window;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct t_found_file
{
	char *matched_filter;
	char *path;
} found_file;

typedef struct t_file_match
{
	found_file file;
	s16 file_error;
	s32 file_size;
	
	u32 line_nr;
	s32 word_match_offset;
	s32 word_match_length;
	s32 word_match_offset_x; // highlight render offset
	s32 word_match_width; // highlight render width
	char *line_info; // will be null when no match is found
} file_match;

typedef struct t_search_info
{
	u64 file_count;
	u64 dir_count;
} search_info;

typedef struct t_search_result
{
	array work_queue;
	array files;
	array matches;
	s32 match_count;
	u64 find_duration_us;
	array errors;
	bool show_error_message; // error occured
	bool found_file_matches; // found/finding file matches
	s32 files_searched;
	s32 files_matched;
	s32 search_result_source_dir_len;
	bool match_found; // found text match
	mutex mutex;
	bool walking_file_system;
	bool cancel_search;
	bool done_finding_matches;
	s32 search_id;
	u64 start_time;
	bool done_finding_files;
	memory_bucket mem_bucket;
	bool is_command_line_search;
	bool threads_closed;
	search_info search_info;
	char *export_path;
	char *file_filter;
	char *directory_to_search;
	char *text_to_find;
	s32 max_thread_count;
	s32 max_file_size;
	bool is_recursive;
} search_result;

typedef struct t_find_text_args
{
	file_match file;
	search_result *search_result_buffer;
} find_text_args;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct t_file_content
{
	s64 content_length;
	void *content;
	s16 file_error;
} file_content;

typedef enum t_time_type
{
	TIME_FULL,     // realtime
	TIME_THREAD,   // run time for calling thread
	TIME_PROCESS,  // run time for calling process
} time_type;

typedef enum t_time_precision
{
	TIME_NS, // nanoseconds
	TIME_US, // microseconds
	TIME_MILI_S, // miliseconds
	TIME_S,  // seconds
} time_precision;

typedef struct t_cpu_info
{
	s32 model;
	char model_name[255];
	float32 frequency;
	u32 cache_size;
	u32 cache_alignment;
} cpu_info;

typedef enum t_file_dialog_type
{
	OPEN_FILE,
	OPEN_DIRECTORY,
	SAVE_FILE,
} file_dialog_type;

typedef enum t_file_open_error
{
	FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS = 1,
	FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM = 2,
	FILE_ERROR_NO_ACCESS = 3,
	FILE_ERROR_NOT_FOUND = 4,
	FILE_ERROR_CONNECTION_ABORTED = 5,
	FILE_ERROR_CONNECTION_REFUSED = 6,
	FILE_ERROR_NETWORK_DOWN = 7,
	FILE_ERROR_REMOTE_IO_ERROR = 8,
	FILE_ERROR_STALE = 9, // NFS server file is removed/renamed
	FILE_ERROR_GENERIC = 10,
	FILE_ERROR_TOO_BIG = 11,
} file_open_error;

struct open_dialog_args
{
	char *buffer;
	char *file_filter;
	char *start_path;
	char *default_save_file_extension;
	file_dialog_type type;
};

typedef struct t_list_file_args
{
	array *list;
	char *start_dir;
	char *pattern;
	bool recursive;
	bool include_directories;
	bool *state;
	bool *is_cancelled;
	memory_bucket *bucket;
	search_info *info;
} list_file_args;

typedef enum t_cursor_type
{
	CURSOR_DEFAULT,
	CURSOR_POINTER,
	CURSOR_TEXT,
	CURSOR_DRAG,
} cursor_type;

typedef struct t_vec2
{
	s32 x;
	s32 y;
} vec2;

typedef struct t_backbuffer_pixel
{
	s32 color;
	u8 depth; 
} backbuffer_pixel;

typedef struct t_backbuffer
{
	s32 width;
	s32 height;
	u8 *buffer; // 4bytes color + 1byte depth
#ifdef OS_WIN
	BITMAPINFO bitmapInfo;
#endif
#ifdef OS_LINUX
	XImage * s_image;
#endif
} backbuffer;

// NOT IMPLEMENTED ON LINUX: USE FLAGS_NONE
typedef enum t_window_flags
{
	FLAGS_NONE = 0,
	FLAGS_BORDERLESS = 1,
	FLAGS_TOPMOST = 2,
	FLAGS_GLOBAL_MOUSE = 4,
	FLAGS_HIDDEN = 8,
	FLAGS_NO_TASKBAR = 16,
} window_flags;
// NOT IMPLEMENTED ON LINUX: USE FLAGS_NONE

platform_window *main_window = 0;
platform_window *settings_window = 0;

bool platform_window_is_valid(platform_window *window);

#define platform_open_window(name, width, height, max_w, max_h, min_w, min_h) platform_open_window_ex(name,width,height,max_w,max_h,min_w,min_h, 0)
platform_window platform_open_window_ex(char *name, u16 width, u16 height, u16 max_w, u16 max_h, u16 min_w, u16 min_h, s32 flags);
void platform_get_focus(platform_window *window);
void platform_show_window(platform_window *window);
void platform_hide_window(platform_window *window);
bool platform_set_clipboard(platform_window *window, char *buffer);
bool platform_get_clipboard(platform_window *window, char *buffer);
void platform_window_set_size(platform_window *window, u16 width, u16 height);
void platform_window_set_position(platform_window *window, u16 x, u16 y);
void platform_destroy_window(platform_window *window);
void platform_handle_events(platform_window *window, mouse_input *mouse, keyboard_input *keyboard);
void platform_window_swap_buffers(platform_window *window);
void platform_set_cursor(platform_window *window, cursor_type type);
void platform_window_set_title(platform_window *window, char *name);
file_content platform_read_file_content(char *path, const char *mode);
s32 platform_get_file_size(char *path);
bool platform_write_file_content(char *path, const char *mode, char *buffer, s32 len);
void platform_destroy_file_content(file_content *content);
bool get_active_directory(char *buffer);
bool set_active_directory(char *path);
void platform_show_message(platform_window *window, char *message, char *title);
array get_filters(char *filter);
void platform_list_files_block(array *list, char *start_dir, array filters, bool recursive, memory_bucket *bucket, bool include_directories, bool *is_cancelled, search_info *info);
void platform_list_files(array *list, char *start_dir, char *filter, bool recursive, memory_bucket *bucket, bool *is_cancelled, bool *state, search_info *info);
void platform_open_file_dialog(file_dialog_type type, char *buffer, char *file_filter, char *start_path);
bool platform_get_mac_address(char *buffer, s32 buf_size);
bool is_platform_in_darkmode();
void *platform_open_file_dialog_block(void *arg);
char *platform_get_full_path(char *file);
void platform_open_url(char *command);
bool platform_send_http_request(char *url, char *params, char *response_buffer);
void platform_run_command(char *command);
void platform_window_make_current(platform_window *window);
void platform_init(int argc, char **argv);
void platform_destroy();
void platform_setup_backbuffer(platform_window *window);
void platform_setup_renderer();
void platform_set_icon(platform_window *window, image *img);
void platform_autocomplete_path(char *buffer, bool want_dir);
bool platform_directory_exists(char *path);
void platform_create_directory(char *path);
bool platform_file_exists(char *path);
void platform_show_alert(char *title, char *message);
char *get_config_save_location(char *buffer);
char *get_file_extension(char *path);
void get_name_from_path(char *buffer, char *path);
void get_directory_from_path(char *buffer, char *path);
vec2 platform_get_window_size(platform_window *window);
s32 filter_matches(array *filters, char *string, char **matched_filter);
void platform_delete_file(char *path);

u64 platform_get_time(time_type time_type, time_precision precision);
s32 platform_get_memory_size();
s32 platform_get_cpu_count();

u64 string_to_u64(char *str);
u32 string_to_u32(char *str);
u16 string_to_u16(char *str);
u8 string_to_u8(char *str);

s64 string_to_s64(char *str);
s32 string_to_s32(char *str);
s16 string_to_s16(char *str);
s8 string_to_s8(char *str);

s8 string_to_f32(char *str);
s8 string_to_f64(char *str);

#endif