#include <windows.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <sysinfoapi.h>
#include <wingdi.h>
#include <errno.h>

struct t_platform_window
{
	HWND *window_handle;
	HDC hdc;
	HGLRC gl_context;
	
	s32 width;
	s32 height;
	u8 is_open;
	u8 has_focus;
};

extern BOOL GetPhysicallyInstalledSystemMemory(PULONGLONG TotalMemoryInKilobytes);

platform_window platform_open_window(char *name, u16 width, u16 height, u16 max_w, u16 max_h)
{
	
}

void platform_window_set_size(platform_window *window, u16 width, u16 height)
{
	
}

void platform_close_window(platform_window *window)
{
	
}

void platform_destroy_window(platform_window *window)
{
	
}

void platform_handle_events(platform_window *window, mouse_input *mouse, keyboard_input *keyboard)
{
	
}

void platform_window_swap_buffers(platform_window *window)
{
	SwapBuffers(window->hdc);
}

file_content platform_read_file_content(char *path, const char *mode)
{
	file_content result;
	result.content = 0;
	result.content_length = 0;
	result.file_error = 0;
	
	FILE *file = fopen(path, mode);
	if (!file) 
	{
		// TODO(Aldrik): maybe handle more of these so we can give users more info about what happened.
		// http://man7.org/linux/man-pages/man3/errno.3.html
		if (errno == EMFILE)
			result.file_error = FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS;
		else if (errno == ENFILE)
			result.file_error = FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM;
		else if (errno == EACCES)
			result.file_error = FILE_ERROR_NO_ACCESS;
		else if (errno == EPERM)
			result.file_error = FILE_ERROR_NO_ACCESS;
		else if (errno == ENOENT)
			result.file_error = FILE_ERROR_NOT_FOUND;
		else if (errno == ECONNABORTED)
			result.file_error = FILE_ERROR_CONNECTION_ABORTED;
		else if (errno == ECONNREFUSED)
			result.file_error = FILE_ERROR_CONNECTION_REFUSED;
		else if (errno == ENETDOWN)
			result.file_error = FILE_ERROR_NETWORK_DOWN;
		else
			printf("ERROR: %d\n", errno);
		
		goto done_failure;
	}
	
	fseek(file, 0 , SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	// if file i empty alloc 1 byte
	s32 length_to_alloc = length+1;
	
	result.content = mem_alloc(length_to_alloc);
	if (!result.content) goto done;
	
	fread(result.content, 1, length, file);
	result.content_length = length;
	
	((char*)result.content)[length] = 0;
	
	done:
	fclose(file);
	done_failure:
	return result;
}

u8 platform_write_file_content(char *path, const char *mode, char *buffer, s32 len)
{
	u8 result = false;
	
	FILE *file = fopen(path, mode);
	
	if (!file)
	{
		goto done_failure;
	}
	else
	{
		fprintf(file, buffer);
	}
	
	//done:
	fclose(file);
	done_failure:
	return result;
}

void platform_destroy_file_content(file_content *content)
{
	assert(content);
	mem_free(content->content);
}

u8 get_active_directory(char *buffer)
{
	return GetCurrentDirectory(MAX_INPUT_LENGTH, buffer);
}

u8 set_active_directory(char *path)
{
	return SetCurrentDirectory(path);
}

void platform_list_files_block(array *list, char *start_dir, char *filter, u8 recursive)
{
	
}

void platform_list_files(array *list, char *start_dir, char *filter, u8 recursive, u8 *state)
{
	
}

void platform_open_file_dialog(file_dialog_type type, char *buffer, char *file_filter, char *start_path)
{
	struct open_dialog_args *args = mem_alloc(sizeof(struct open_dialog_args));
	args->buffer = buffer;
	args->type = type;
	args->file_filter = file_filter;
	args->start_path = start_path;
	
	thread thr;
	thr.valid = false;
	
	while (!thr.valid)
		thr = thread_start(platform_open_file_dialog_block, args);
	thread_detach(&thr);
}

static void* platform_open_file_dialog_dd(void *data)
{
	// TODO(Aldrik): implement
}

void *platform_open_file_dialog_block(void *arg)
{
	thread thr = thread_start(platform_open_file_dialog_dd, arg);
	thread_join(&thr);
	mem_free(arg);
	return 0;
}

char *platform_get_full_path(char *file)
{
	char *buf = mem_alloc(PATH_MAX);
	GetFullPathNameA(file, PATH_MAX, buf, NULL);
	return buf;
}

void platform_open_url(char *command)
{
	platform_run_command(command);
}

void platform_run_command(char *command)
{
	// might be start instead of open
	ShellExecuteA(NULL, "open", command, NULL, NULL, SW_SHOWDEFAULT);
}

void platform_window_make_current(platform_window *window)
{
	wglMakeCurrent(window->hdc, window->gl_context);
}

void platform_init()
{
	
}

void platform_set_icon(platform_window *window, image *img)
{
	
}

u64 platform_get_time(time_type time_type, time_precision precision)
{
	LARGE_INTEGER val_v;
	BOOL result = QueryPerformanceCounter(&val_v);
	
	u64 val = val_v.QuadPart;
	
	if (precision == TIME_NS)
	{
		return val*1000;
	}
	if (precision == TIME_US)
	{
		return val;
	}
	if (precision == TIME_MILI_S)
	{
		return val/1000;
	}
	if (precision == TIME_S)
	{
		return val*1000000;
	}
	return val;
}

s32 platform_get_memory_size()
{
	u64 result;
	GetPhysicallyInstalledSystemMemory(&result);
	return result;
}

s32 platform_get_cpu_count()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	
	return info.dwNumberOfProcessors;
}

cpu_info platform_get_cpu_info()
{
	// https://docs.microsoft.com/en-us/windows/desktop/api/sysinfoapi/ns-sysinfoapi-_system_info same as above
	cpu_info info;
	return info;
}

u64 string_to_u64(char *str)
{
	return (u64)strtoull(str, 0, 10);
}

u32 string_to_u32(char *str)
{
	return (u32)strtoul(str, 0, 10);
}

u16 string_to_u16(char *str)
{
	return (u16)strtoul(str, 0, 10);
}

u8 string_to_u8(char *str)
{
	return (u8)strtoul(str, 0, 10);
}

s64 string_to_s64(char *str)
{
	return (s64)strtoull(str, 0, 10);
}

s32 string_to_s32(char *str)
{
	return (s32)strtoul(str, 0, 10);
}

s16 string_to_s16(char *str)
{
	return (s16)strtoul(str, 0, 10);
}

s8 string_to_s8(char *str)
{
	return (s8)strtoul(str, 0, 10);
}