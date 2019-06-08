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
	TIME_MS, // miliseconds
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
	file_dialog_type type;
};

bool platform_cancel_search = false;

platform_window platform_open_window(char *name, u16 width, u16 height);
void platform_close_window(platform_window *window);
void platform_handle_events(platform_window *window, mouse_input *mouse, keyboard_input *keyboard);
void platform_window_swap_buffers(platform_window *window);
file_content platform_read_file_content(char *path, const char *mode);
bool platform_write_file_content(char *path, const char *mode, char *buffer, s32 len);
void platform_destroy_file_content(file_content *content);
bool get_active_directory(char *buffer);
bool set_active_directory(char *path);
void platform_list_files(array *list, char *start_dir, char *filter, bool recursive, bool *state);
void platform_open_file_dialog(file_dialog_type type, char *buffer);
char *platform_get_full_path(char *file);
void *platform_open_file_dialog_d(void *arg);

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