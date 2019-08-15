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

#ifndef INCLUDE_PLATFORM
#define INCLUDE_PLATFORM

typedef struct t_platform_window platform_window;

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

typedef struct t_found_file
{
	char *matched_filter;
	char *path;
} found_file;

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
} file_open_error;

struct open_dialog_args
{
	char *buffer;
	char *file_filter;
	char *start_path;
	file_dialog_type type;
};

typedef struct t_list_file_args
{
	array *list;
	char *start_dir;
	char *pattern;
	u8 recursive;
	u8 include_directories;
	u8 *state;
} list_file_args;

typedef enum t_drag_drop_state
{
	DRAG_DROP_NONE,
	DRAG_DROP_ENTER,
	DRAG_DROP_LEAVE,
	DRAG_DROP_FINISHED,
	DRAG_DROP_ACTIVE,
} drag_drop_state;

typedef enum t_cursor_type
{
	CURSOR_DEFAULT,
	CURSOR_POINTER,
} cursor_type;

struct drag_drop_info
{
	s32 state;
	char *path;
};

memory_bucket_collection global_platform_memory_bucket;
u8 platform_cancel_search = false;

u8 platform_window_is_valid(platform_window *window);
platform_window platform_open_window(char *name, u16 width, u16 height, u16 max_w, u16 max_h);
u8 platform_get_clipboard(platform_window *window, char *buffer);
void platform_window_set_size(platform_window *window, u16 width, u16 height);
void platform_close_window(platform_window *window);
void platform_destroy_window(platform_window *window);
void platform_handle_events(platform_window *window, mouse_input *mouse, keyboard_input *keyboard);
void platform_window_swap_buffers(platform_window *window);
void platform_set_cursor(platform_window *window, cursor_type type);
void platform_window_set_title(platform_window *window, char *name);
file_content platform_read_file_content(char *path, const char *mode);
u8 platform_write_file_content(char *path, const char *mode, char *buffer, s32 len);
void platform_destroy_file_content(file_content *content);
u8 get_active_directory(char *buffer);
u8 set_active_directory(char *path);
void platform_destroy_list_file_result(array *files);
void platform_show_message(platform_window *window, char *message, char *title);
array get_filters(char *filter);
void platform_list_files_block(array *list, char *start_dir, array filters, u8 recursive, u8 include_directories);
void platform_list_files(array *list, char *start_dir, char *filter, u8 recursive, u8 *state);
void platform_open_file_dialog(file_dialog_type type, char *buffer, char *file_filter, char *start_path);
void *platform_open_file_dialog_block(void *arg);
char *platform_get_full_path(char *file);
void platform_open_url(char *command);
void platform_run_command(char *command);
void platform_window_make_current(platform_window *window);
void platform_init();
void platform_destroy();
void platform_set_icon(platform_window *window, image *img);
void platform_autocomplete_path(char *buffer, bool want_dir);
u8 platform_directory_exists(char *path);
u8 platform_file_exists(char *path);
void platform_show_alert(char *title, char *message);

u64 platform_get_time(time_type time_type, time_precision precision);
s32 platform_get_memory_size();
s32 platform_get_cpu_count();
cpu_info platform_get_cpu_info();

u64 string_to_u64(char *str);
u32 string_to_u32(char *str);
u16 string_to_u16(char *str);
u8 string_to_u8(char *str);

s64 string_to_s64(char *str);
s32 string_to_s32(char *str);
s16 string_to_s16(char *str);
s8 string_to_s8(char *str);

#endif