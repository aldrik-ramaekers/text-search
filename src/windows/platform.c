#include <windows.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <sysinfoapi.h>
#include <wingdi.h>
#include <errno.h>

struct t_platform_window
{
	HWND window_handle;
	HDC hdc;
	HGLRC gl_context;
	WNDCLASS window_class;
	
	s32 width;
	s32 height;
	u8 is_open;
	u8 has_focus;
};

extern BOOL GetPhysicallyInstalledSystemMemory(PULONGLONG TotalMemoryInKilobytes);

static HINSTANCE instance;
platform_window *current_window_to_handle;
keyboard_input *current_keyboard_to_handle;
mouse_input *current_mouse_to_handle;

int cmd_show;
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine, int nCmdShow)
{
	instance = hInstance;
	cmd_show = nCmdShow;
	return main_loop();
}

LRESULT CALLBACK main_window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	
	if (message == WM_SIZE)
	{
		u32 width = lparam&0xFFFF;
		u32 height = lparam>>16;
		
		// CRASHES:
		//current_window_to_handle->width = width;
		//current_window_to_handle->height = height;
	}
	else if (message == WM_LBUTTONDOWN || 
			 message == WM_RBUTTONDOWN ||
			 message == WM_MBUTTONDOWN ||
			 message == WM_MOUSEWHEEL)
	{
		u8 is_left_down = wparam & MK_LBUTTON;
		u8 is_right_down = wparam & MK_RBUTTON;
		u8 is_middle_down = wparam & MK_MBUTTON;
		
		if (message == WM_MOUSEWHEEL)
		{
			s32 scroll_val = wparam>>16;
			
			if (scroll_val > 120)
				current_mouse_to_handle->scroll_state = SCROLL_UP;
			else
				current_mouse_to_handle->scroll_state = SCROLL_DOWN;
		}
		
		if (is_left_down)
		{
			current_mouse_to_handle->left_state |= MOUSE_DOWN;
			current_mouse_to_handle->left_state |= MOUSE_CLICK;
		}
		if (is_right_down)
		{
			current_mouse_to_handle->right_state |= MOUSE_DOWN;
			current_mouse_to_handle->right_state |= MOUSE_CLICK;
		}
	}
	else if (message == WM_LBUTTONUP || 
			 message == WM_RBUTTONUP ||
			 message == WM_MBUTTONUP)
	{
		u8 is_left_up = message == WM_LBUTTONUP;
		u8 is_right_up = message == WM_RBUTTONUP;
		u8 is_middle_up = message == WM_MBUTTONUP;
		
		if (is_left_up)
		{
			current_mouse_to_handle->left_state = MOUSE_RELEASE;
		}
		if (is_right_up)
		{
			current_mouse_to_handle->right_state = MOUSE_RELEASE;
		}
	}
	else if (message == WM_MOUSEMOVE)
	{
		s32 x = lparam&0xFFFF;
		s32 y = lparam>>16;
		
		current_mouse_to_handle->x = x;
		current_mouse_to_handle->y = y;
	}
	else if (message == WM_GETMINMAXINFO)
	{
		
	}
	else if (message == WM_DESTROY)
	{
		current_window_to_handle->is_open = false;
	}
	else if (message == WM_CLOSE)
	{
		current_window_to_handle->is_open = false;
	}
	else
	{
		result = DefWindowProc(window, message, wparam, lparam);
	}
	
	return result;
}

platform_window platform_open_window(char *name, u16 width, u16 height, u16 max_w, u16 max_h)
{
	platform_window window;
	window.window_handle = 0;
	window.hdc = 0;
	
    memset(&window.window_class, 0, sizeof(WNDCLASS));
	window.window_class.style = CS_OWNDC;
	window.window_class.lpfnWndProc = main_window_callback;
	window.window_class.hInstance = instance;
	window.window_class.lpszClassName = name;
    window.window_class.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    window.window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	
	if (RegisterClass(&window.window_class))
	{
		window.window_handle = CreateWindowEx(0,
											  window.window_class.lpszClassName,
											  name,
											  WS_VISIBLE|WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX,
											  CW_USEDEFAULT,
											  CW_USEDEFAULT,
											  width,
											  height,
											  0,
											  0,
											  instance,
											  0);
		
		if (window.window_handle)
		{
			window.hdc = GetDC(window.window_handle);
			
			PIXELFORMATDESCRIPTOR format;
			memset(&format, 0, sizeof(PIXELFORMATDESCRIPTOR));
			format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			format.nVersion = 1;
			format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
			format.cColorBits = 24;
			format.cAlphaBits = 8;
			format.iLayerType = PFD_MAIN_PLANE; // PFD_TYPE_RGBA
			s32 suggested_format_index = ChoosePixelFormat(window.hdc, &format);
			
			PIXELFORMATDESCRIPTOR actual_format;
			DescribePixelFormat(window.hdc, suggested_format_index, sizeof(actual_format), &actual_format);
			SetPixelFormat(window.hdc, suggested_format_index, &actual_format);
			
			window.gl_context = wglCreateContext(window.hdc);
			
			static HGLRC share_list = 0;
			if (share_list == 0)
			{
				share_list = window.gl_context;
			}
			else
			{
				wglShareLists(share_list, window.gl_context);
			}
			
			wglMakeCurrent(window.hdc, window.gl_context);
			
			ShowWindow(window.window_handle, cmd_show);
			
			// blending
			glEnable(GL_DEPTH_TEST);
			//glDepthMask(true);
			//glClearDepth(50);
			glDepthFunc(GL_LEQUAL);
			
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			// setup multisampling
#if 0
			glEnable(GL_ALPHA_TEST);
			glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glEnable(GL_SAMPLE_ALPHA_TO_ONE);
			glEnable(GL_MULTISAMPLE);
			glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
#endif
			
			// TODO: is this correct?
			// https://stackoverflow.com/questions/5627229/sub-pixel-drawing-with-opengl
			//glHint(GL_POINT_SMOOTH, GL_NICEST);
			//glHint(GL_LINE_SMOOTH, GL_NICEST);
			//glHint(GL_POLYGON_SMOOTH, GL_NICEST);
			
			//glEnable(GL_SMOOTH);
			//glEnable(GL_POINT_SMOOTH);
			//glEnable(GL_LINE_SMOOTH);
			//glEnable(GL_POLYGON_SMOOTH);
			//////////////////
			
			window.is_open = true;
			window.width = width;
			window.height = height;
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, width, height, 0, -1, 1);
			
			//GLint m_viewport[4];
			//glGetIntegerv( GL_VIEWPORT, m_viewport );
			//printf("%d %d %d %d\n", m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
			
			glMatrixMode(GL_MODELVIEW);
		}
	}
	
	return window;
}

void platform_window_set_size(platform_window *window, u16 width, u16 height)
{
	RECT rec;
	GetWindowRect(window->window_handle, &rec);
	MoveWindow(window->window_handle, rec.left, rec.top, width, height, FALSE);
}

u8 platform_window_is_valid(platform_window *window)
{
	return window->hdc && window->window_handle;
}

void platform_close_window(platform_window *window)
{
	ReleaseDC(window->window_handle, window->hdc);
	CloseWindow(window->window_handle);
	DestroyWindow(window->window_handle);
	UnregisterClassA(window->window_class.lpszClassName, instance);
	window->hdc = 0;
	window->window_handle = 0;
}

void platform_destroy_window(platform_window *window)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(window->gl_context);
}

void platform_handle_events(platform_window *window, mouse_input *mouse, keyboard_input *keyboard)
{
	current_window_to_handle = window;
	current_keyboard_to_handle = keyboard;
	current_mouse_to_handle = mouse;
	
	mouse->left_state &= ~MOUSE_CLICK;
	mouse->right_state &= ~MOUSE_CLICK;
	mouse->left_state &= ~MOUSE_RELEASE;
	mouse->right_state &= ~MOUSE_RELEASE;
	memset(keyboard->input_keys, 0, MAX_KEYCODE);
	mouse->move_x = 0;
	mouse->move_y = 0;
	
	MSG message;
	while(PeekMessageA(&message, window->window_handle, 0, 0, TRUE))
	{
		TranslateMessage(&message); 
		DispatchMessage(&message); 
	}
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
	assert(list);
	
	s32 len = strlen(filter);
	
	char *subdirname_buf = mem_alloc(PATH_MAX);
	
	WIN32_FIND_DATAA file_info;
	HWND handle = FindFirstFileA(start_dir, &file_info);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
		return;
	}
	
	do
	{
		char *name = file_info.cFileName;
		
		if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && recursive)
		{
			if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))
				continue;
			
			strcpy(subdirname_buf, start_dir);
			strcat(subdirname_buf, name);
			strcat(subdirname_buf, "/");
			
			// is directory
			platform_list_files_block(list, subdirname_buf, filter, recursive);
		}
		else if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) || 
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
		{
			// is file
			char *buf = mem_alloc(PATH_MAX);
			sprintf(buf, "%s%s",start_dir, name);
			
			found_file f;
			f.path = buf;
			f.matched_filter = mem_alloc(len);
			strcpy(f.matched_filter, filter);
			array_push_size(list, &f, sizeof(found_file));
		}
	}
	while (FindNextFile(handle, &file_info) != 0);
	
	FindClose(handle);
	
	mem_free(subdirname_buf);
}

void* platform_list_files_t_t(void *args)
{
	list_file_args *info = args;
	platform_list_files_block(info->list, info->start_dir, info->pattern, info->recursive);
	mem_free(info);
	return 0;
}

void *platform_list_files_t(void *args)
{
	list_file_args *info = args;
	
	// TODO(Aldrik): hardcoded max filter length
	s32 max_filter_len = MAX_PATH_LENGTH;
	
	array filters = array_create(max_filter_len);
	
	char current_filter[max_filter_len];
	s32 filter_len = 0;
	
	array *list = info->list;
	char *start_dir = info->start_dir;
	char *pattern = info->pattern;
	u8 recursive = info->recursive;
	
	while(*pattern)
	{
		char ch = *pattern;
		
		if (ch == ',')
		{
			current_filter[filter_len] = 0;
			array_push(&filters, current_filter);
			filter_len = 0;
		}
		else
		{
			// TODO(Aldrik): show error and dont continue search
			assert(filter_len < MAX_PATH_LENGTH);
			
			current_filter[filter_len++] = ch;
		}
		
		pattern++;
	}
	current_filter[filter_len] = 0;
	array_push(&filters, current_filter);
	
	array threads = array_create(sizeof(thread));
	array_reserve(&threads, filters.length);
	
	for (s32 i = 0; i < filters.length; i++)
	{
		char *filter = array_at(&filters, i);
		
		thread thr;
		thr.valid = false;
		
		list_file_args *args_2 = mem_alloc(sizeof(list_file_args));
		if (args_2)
		{
			args_2->list = list;
			args_2->start_dir = start_dir;
			args_2->pattern = filter;
			args_2->recursive = recursive;
			
			thr = thread_start(platform_list_files_t_t, args_2);
		}
		
		if (platform_cancel_search) break;
		
		if (thr.valid)
		{
			array_push(&threads, &thr);
		}
		else
		{
			i--;
		}
	}
	
	for (s32 i = 0; i < threads.length; i++)
	{
		thread* thr = array_at(&threads, i);
		thread_join(thr);
	}
	
	if (!platform_cancel_search)
		*(info->state) = !(*info->state);
	
	array_destroy(&threads);
	array_destroy(&filters);
	mem_free(args);
	
	return 0;
}

void platform_list_files(array *list, char *start_dir, char *filter, u8 recursive, u8 *state)
{
	platform_cancel_search = false;
	list_file_args *args = mem_alloc(sizeof(list_file_args));
	args->list = list;
	args->start_dir = start_dir;
	args->pattern = filter;
	args->recursive = recursive;
	args->state = state;
	
	thread thr = thread_start(platform_list_files_t, args);
	thread_detach(&thr);
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
	// TODO(Aldrik): implement
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