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

#include <locale.h>
#include <windows.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <sysinfoapi.h>
#include <wingdi.h>
#include <errno.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <shlobj.h>

struct t_platform_window
{
	HWND window_handle;
	HDC hdc;
	HGLRC gl_context;
	WNDCLASS window_class;
	
    s32 min_width;
	s32 min_height;
	s32 max_width;
	s32 max_height;
	
	// shared window properties
	s32 width;
	s32 height;
	bool is_open;
	bool has_focus;
	cursor_type curr_cursor_type;
	cursor_type next_cursor_type;
};

extern BOOL GetPhysicallyInstalledSystemMemory(PULONGLONG TotalMemoryInKilobytes);

LARGE_INTEGER perf_frequency;
static HINSTANCE instance;
platform_window *current_window_to_handle;
keyboard_input *current_keyboard_to_handle;
mouse_input *current_mouse_to_handle;

int cmd_show;

bool platform_get_clipboard(platform_window *window, char *buffer)
{
	if (!OpenClipboard(NULL))
		return false;
	
	if (!IsClipboardFormatAvailable(CF_TEXT)) 
	{
		CloseClipboard();
		return false;
	}
	
	char *clip_str = GetClipboardData(CF_TEXT);
	if (!clip_str)
	{
		CloseClipboard();
		return false;
	}
	
	strncpy(buffer, clip_str, MAX_INPUT_LENGTH);
	
	CloseClipboard();
	return true;
}

bool platform_set_clipboard(platform_window *window, char *buffer)
{
	HGLOBAL clipboard_data;
	s32 buffer_len = strlen(buffer)+1;
	
	if (!OpenClipboard(NULL))
		return false;
	
	clipboard_data = GlobalAlloc(GMEM_MOVEABLE, buffer_len);
	if (clipboard_data)
	{
		void *addr = GlobalLock(clipboard_data);
		memcpy(addr, buffer, buffer_len);
		GlobalUnlock(clipboard_data);
		
		if (!EmptyClipboard())
		{
			CloseClipboard();
			return false;
		}
		else
		{	
			SetClipboardData(CF_TEXT, addr);
		}
	}
	{
		CloseClipboard();
		return false;
	}
	
	CloseClipboard();
	return true;
}

inline void platform_show_alert(char *title, char *message)
{
	// not implemented
}

inline void platform_destroy()
{
	assets_destroy();
	
#if defined(MODE_DEVELOPER)
	memory_print_leaks();
#endif
}

bool is_platform_in_darkmode()
{
	char *key = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize\\";
	
	HKEY result;
	LSTATUS o = RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_READ, &result);
	
	if (o == 0)
	{
		BYTE data;
		DWORD len = 4;
		RegQueryValueExA(result, "AppsUseLightTheme", NULL, NULL, &data, &len);
		
		if (data == 1) return true;
	}
	
	return false;
}

inline void platform_set_cursor(platform_window *window, cursor_type type)
{
	if (window->next_cursor_type != type)
	{
		window->next_cursor_type = type;
	}
}

bool platform_directory_exists(char *path)
{
	return PathFileExistsA(path) == TRUE;
}

static void create_key_tables()
{
	keycode_map[0x30] = KEY_0;
	keycode_map[0x31] = KEY_1;
	keycode_map[0x32] = KEY_2;
	keycode_map[0x33] = KEY_3;
	keycode_map[0x34] = KEY_4;
	keycode_map[0x35] = KEY_5;
	keycode_map[0x36] = KEY_6;
	keycode_map[0x37] = KEY_7;
	keycode_map[0x38] = KEY_8;
	keycode_map[0x39] = KEY_9;
	keycode_map[0x41] = KEY_A;
	keycode_map[0x42] = KEY_B;
	keycode_map[0x43] = KEY_C;
	keycode_map[0x44] = KEY_D;
	keycode_map[0x45] = KEY_E;
	keycode_map[0x46] = KEY_F;
	keycode_map[0x47] = KEY_G;
	keycode_map[0x48] = KEY_H;
	keycode_map[0x49] = KEY_I;
	keycode_map[0x4A] = KEY_J;
	keycode_map[0x4B] = KEY_K;
	keycode_map[0x4C] = KEY_L;
	keycode_map[0x4D] = KEY_M;
	keycode_map[0x4E] = KEY_N;
	keycode_map[0x4F] = KEY_O;
	keycode_map[0x50] = KEY_P;
	keycode_map[0x51] = KEY_Q;
	keycode_map[0x52] = KEY_R;
	keycode_map[0x53] = KEY_S;
	keycode_map[0x54] = KEY_T;
	keycode_map[0x55] = KEY_U;
	keycode_map[0x56] = KEY_V;
	keycode_map[0x57] = KEY_W;
	keycode_map[0x58] = KEY_X;
	keycode_map[0x59] = KEY_Y;
	keycode_map[0x5A] = KEY_Z;
	
	keycode_map[VK_OEM_7] = KEY_APOSTROPHE;
	keycode_map[VK_OEM_102] = KEY_BACKSLASH;
	keycode_map[VK_OEM_COMMA] = KEY_COMMA;
	keycode_map[VK_OEM_3] = KEY_GRAVE_ACCENT;
	keycode_map[VK_OEM_4] = KEY_LEFT_BRACKET;
	keycode_map[VK_OEM_MINUS] = KEY_MINUS;
	keycode_map[VK_OEM_PERIOD] = KEY_PERIOD;
	
	keycode_map[VK_BACK] = KEY_BACKSPACE;
	keycode_map[VK_DELETE] = KEY_DELETE;
	keycode_map[VK_END] = KEY_END;
	keycode_map[VK_RETURN] = KEY_ENTER;
	keycode_map[VK_ESCAPE] = KEY_ESCAPE;
	keycode_map[VK_HOME] = KEY_HOME;
	keycode_map[VK_INSERT] = KEY_INSERT;
	keycode_map[VK_MENU] = KEY_MENU;
	keycode_map[VK_NEXT] = KEY_PAGE_DOWN;
	keycode_map[VK_PRIOR] = KEY_PAGE_UP;
	keycode_map[VK_PAUSE] = KEY_PAUSE;
	keycode_map[VK_TAB] = KEY_TAB;
	keycode_map[VK_CAPITAL] = KEY_CAPS_LOCK;
	keycode_map[VK_NUMLOCK] = KEY_NUM_LOCK;
	keycode_map[VK_SCROLL] = KEY_SCROLL_LOCK;
	keycode_map[0x70] = KEY_F1;
	keycode_map[0x71] = KEY_F2;
	keycode_map[0x72] = KEY_F3;
	keycode_map[0x73] = KEY_F4;
	keycode_map[0x74] = KEY_F5;
	keycode_map[0x75] = KEY_F6;
	keycode_map[0x76] = KEY_F7;
	keycode_map[0x77] = KEY_F8;
	keycode_map[0x78] = KEY_F9;
	keycode_map[0x79] = KEY_F10;
	keycode_map[0x7A] = KEY_F11;
	keycode_map[0x7B] = KEY_F12;
	keycode_map[0x7C] = KEY_F13;
	keycode_map[0x7D] = KEY_F14;
	keycode_map[0x7E] = KEY_F15;
	keycode_map[0x7F] = KEY_F16;
	keycode_map[0x80] = KEY_F17;
	keycode_map[0x81] = KEY_F18;
	keycode_map[0x82] = KEY_F19;
	keycode_map[0x83] = KEY_F20;
	keycode_map[0x84] = KEY_F21;
	keycode_map[0x85] = KEY_F22;
	keycode_map[0x86] = KEY_F23;
	keycode_map[0x87] = KEY_F24;
	keycode_map[0x88] = KEY_LEFT_ALT;
	keycode_map[VK_CONTROL] = KEY_LEFT_CONTROL;
	keycode_map[VK_LCONTROL] = KEY_LEFT_CONTROL;
	keycode_map[VK_LSHIFT] = KEY_LEFT_SHIFT;
	keycode_map[VK_LWIN] = KEY_LEFT_SUPER;
	keycode_map[VK_SNAPSHOT] = KEY_PRINT_SCREEN;
	keycode_map[VK_RMENU] = KEY_RIGHT_ALT;
	keycode_map[VK_RCONTROL] = KEY_RIGHT_CONTROL;
	keycode_map[VK_RSHIFT] = KEY_RIGHT_SHIFT;
	keycode_map[VK_RWIN] = KEY_RIGHT_SUPER;
	keycode_map[VK_DOWN] = KEY_DOWN;
	keycode_map[VK_LEFT] = KEY_LEFT;
	keycode_map[VK_RIGHT] = KEY_RIGHT;
	keycode_map[VK_UP] = KEY_UP;
	
	keycode_map[VK_NUMPAD0] = KEY_KP_0;
	keycode_map[VK_NUMPAD1] = KEY_KP_1;
	keycode_map[VK_NUMPAD2] = KEY_KP_2;
	keycode_map[VK_NUMPAD3] = KEY_KP_3;
	keycode_map[VK_NUMPAD4] = KEY_KP_4;
	keycode_map[VK_NUMPAD5] = KEY_KP_5;
	keycode_map[VK_NUMPAD6] = KEY_KP_6;
	keycode_map[VK_NUMPAD7] = KEY_KP_7;
	keycode_map[VK_NUMPAD8] = KEY_KP_8;
	keycode_map[VK_NUMPAD9] = KEY_KP_9;
	keycode_map[VK_ADD] = KEY_KP_ADD;
	keycode_map[VK_DECIMAL] = KEY_KP_DECIMAL;
	keycode_map[VK_DIVIDE] = KEY_KP_DIVIDE;
	keycode_map[VK_MULTIPLY] = KEY_KP_MULTIPLY;
	keycode_map[VK_SUBTRACT] = KEY_KP_SUBTRACT;
}

bool platform_file_exists(char *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
			!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void platform_create_config_directory()
{
	char tmp[PATH_MAX];
	if(SUCCEEDED(SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, tmp)))
	{
		strcat(tmp, "/text-search");
	}
	
	
	if (!platform_directory_exists(tmp))
	{
		CreateDirectoryA(tmp, NULL);
	}
}

char* get_config_save_location(char *buffer)
{
	if(SUCCEEDED(SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, buffer)))
	{
		strcat(buffer, "\\text-search\\config.txt");
		return buffer;
	}
	
	return 0;
}

void platform_show_message(platform_window *window, char *message, char *title)
{
	HWND handle = window ? window->window_handle : NULL;
	MessageBox(handle, message, title, MB_ICONINFORMATION | MB_OK);
}

LRESULT CALLBACK main_window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	
	if (message == WM_SIZE)
	{
		u32 width = lparam&0xFFFF;
		u32 height = lparam>>16;
		
		current_window_to_handle->width = width;
		current_window_to_handle->height = height;
	}
	else if (message == WM_CHAR)
	{
		if (current_keyboard_to_handle->take_input)
		{
			char buf[2];
			char *ch = 0;
			
			char val = (char)wparam;
			
			if (current_keyboard_to_handle->input_mode == INPUT_NUMERIC)
			{
				if (!(val >= 48 && val <= 57))
				{
					ch = 0;
				}
				else
				{
					sprintf(buf, "%c", val);
					ch = buf;
				}
			}
			else if (val >= 32 && val <= 126)
			{
				sprintf(buf, "%c", val);
				ch = buf;
			}
			
			if (ch != 0)
				keyboard_handle_input_string(current_window_to_handle, current_keyboard_to_handle, ch);
		}
	}
	else if (message == WM_MOUSELEAVE)
	{
		current_mouse_to_handle->x = MOUSE_OFFSCREEN;
		current_mouse_to_handle->y = MOUSE_OFFSCREEN;
	}
	else if (message == WM_KILLFOCUS)
	{
		current_mouse_to_handle->x = MOUSE_OFFSCREEN;
		current_mouse_to_handle->y = MOUSE_OFFSCREEN;
		
		current_window_to_handle->has_focus = false;
	}
	else if (message == WM_SETFOCUS)
	{
		current_window_to_handle->has_focus = true;
	}
	else if (message == WM_KEYDOWN)
	{
		s32 key = wparam;
		
		current_keyboard_to_handle->keys[keycode_map[key]] = true;
		current_keyboard_to_handle->input_keys[keycode_map[key]] = true;
		
		if (current_keyboard_to_handle->take_input)
			keyboard_handle_input_string(current_window_to_handle, 
										 current_keyboard_to_handle, 0);
	}
	else if (message == WM_KEYUP)
	{
		s32 key = wparam;
		current_keyboard_to_handle->keys[keycode_map[key]] = false;
		current_keyboard_to_handle->input_keys[keycode_map[key]] = false;
	}
	else if (message == WM_LBUTTONDOWN || 
			 message == WM_RBUTTONDOWN ||
			 message == WM_MBUTTONDOWN ||
			 message == WM_MOUSEWHEEL)
	{
		bool is_left_down = wparam & MK_LBUTTON;
		bool is_right_down = wparam & MK_RBUTTON;
		bool is_middle_down = wparam & MK_MBUTTON;
		
		u64 ev_time = platform_get_time(TIME_FULL, TIME_MILI_S);
		static u64  last_ev_time = 0;
		
		if (message == WM_MOUSEWHEEL)
		{
			s16 scroll_val = wparam>>16;
			
			if (scroll_val < 0)
				current_mouse_to_handle->scroll_state = SCROLL_DOWN;
			else
				current_mouse_to_handle->scroll_state = SCROLL_UP;
		}
		
		if (is_left_down)
		{
			if (ev_time - last_ev_time < 200)
			{
				current_mouse_to_handle->left_state |= MOUSE_DOUBLE_CLICK;
			}
			
			current_mouse_to_handle->left_state |= MOUSE_DOWN;
			current_mouse_to_handle->left_state |= MOUSE_CLICK;
			
			current_mouse_to_handle->total_move_x = 0;
			current_mouse_to_handle->total_move_y = 0;
			last_ev_time = ev_time;
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
		bool is_left_up = message == WM_LBUTTONUP;
		bool is_right_up = message == WM_RBUTTONUP;
		bool is_middle_up = message == WM_MBUTTONUP;
		
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
		current_window_to_handle->curr_cursor_type = -999;
		
		s32 x = lparam&0xFFFF;
		s32 y = lparam>>16;
		
		current_mouse_to_handle->x = x;
		current_mouse_to_handle->y = y;
	}
	else if (message == WM_GETMINMAXINFO)
	{
		MINMAXINFO *info = (MINMAXINFO*)lparam;
		
		info->ptMinTrackSize.x = current_window_to_handle->min_width;
		info->ptMinTrackSize.y = current_window_to_handle->min_height;
		
		if (current_window_to_handle->max_width)
			info->ptMaxTrackSize.x = current_window_to_handle->max_width;
		if (current_window_to_handle->max_height)
			info->ptMaxTrackSize.y = current_window_to_handle->max_height;
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

void platform_window_set_title(platform_window *window, char *name)
{
	SetWindowTextA(window->window_handle, name);
}

platform_window platform_open_window(char *name, u16 width, u16 height, u16 max_w, u16 max_h)
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	platform_window window;
	window.has_focus = true;
	window.window_handle = 0;
	window.hdc = 0;
	window.width = width;
	window.height = height;
	window.min_width = width;
	window.min_height = height;
	window.max_width = max_w;
	window.max_height = max_h;
	window.curr_cursor_type = -1;
	window.next_cursor_type = CURSOR_DEFAULT;
	
	current_window_to_handle = &window;
	
	memset(&window.window_class, 0, sizeof(WNDCLASS));
	window.window_class.style = CS_OWNDC;
	window.window_class.lpfnWndProc = main_window_callback;
	window.window_class.hInstance = instance;
	window.window_class.lpszClassName = name;
	window.window_class.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	//window.window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	
	if (RegisterClass(&window.window_class))
	{
		s32 ex_style = 0;
		s32 style = 0;
		
		if (max_w == 0 && max_h == 0)
		{
			style = WS_SIZEBOX;
		}
		else
		{
			style = WS_THICKFRAME;
		}
		
		window.window_handle = CreateWindowEx(ex_style,
											  window.window_class.lpszClassName,
											  name,
											  WS_VISIBLE|WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX|style,
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
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, width, height, 0, -1, 1);
			
			//GLint m_viewport[4];
			//glGetIntegerv( GL_VIEWPORT, m_viewport );
			//printf("%d %d %d %d\n", m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
			
			glMatrixMode(GL_MODELVIEW);
			
			TRACKMOUSEEVENT track;
			track.cbSize = sizeof(track);
			track.dwFlags = TME_LEAVE;
			track.hwndTrack = window.window_handle;
		}
		else
		{
			platform_show_message(0, "An error occured within Windows, please restart the program.", "Error");
			abort();
		}
	}
	else
	{
		platform_show_message(0, "An error occured within Windows, please restart the program.", "Error");
		abort();
	}
	
	return window;
}

void platform_window_set_size(platform_window *window, u16 width, u16 height)
{
	RECT rec;
	GetWindowRect(window->window_handle, &rec);
	MoveWindow(window->window_handle, rec.left, rec.top, width, height, FALSE);
}

void platform_window_set_position(platform_window *window, u16 x, u16 y)
{
	RECT rec;
	GetWindowRect(window->window_handle, &rec);
	MoveWindow(window->window_handle, x, y, rec.right-rec.left, rec.bottom-rec.top, FALSE);
}

bool platform_window_is_valid(platform_window *window)
{
	return window->hdc && window->window_handle;
}

void platform_destroy_window(platform_window *window)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(window->gl_context);
	
	ReleaseDC(window->window_handle, window->hdc);
	CloseWindow(window->window_handle);
	DestroyWindow(window->window_handle);
	UnregisterClassA(window->window_class.lpszClassName, instance);
	window->hdc = 0;
	window->window_handle = 0;
}

void platform_handle_events(platform_window *window, mouse_input *mouse, keyboard_input *keyboard)
{
	current_window_to_handle = window;
	current_keyboard_to_handle = keyboard;
	current_mouse_to_handle = mouse;
	
	mouse->left_state &= ~MOUSE_CLICK;
	mouse->right_state &= ~MOUSE_CLICK;
	mouse->left_state &= ~MOUSE_DOUBLE_CLICK;
	mouse->right_state &= ~MOUSE_DOUBLE_CLICK;
	mouse->left_state &= ~MOUSE_RELEASE;
	mouse->right_state &= ~MOUSE_RELEASE;
	memset(keyboard->input_keys, 0, MAX_KEYCODE);
	mouse->move_x = 0;
	mouse->move_y = 0;
	mouse->scroll_state = 0;
	
	MSG message;
	while(PeekMessageA(&message, window->window_handle, 0, 0, TRUE))
	{
		TranslateMessage(&message); 
		DispatchMessage(&message); 
	}
	
	glViewport(0, 0, window->width, window->height);
}

void platform_window_swap_buffers(platform_window *window)
{
	// set cursor if changed
	if (window->curr_cursor_type != window->next_cursor_type)
	{
		char *cursor_shape = 0;
		switch(window->next_cursor_type)
		{
			case CURSOR_DEFAULT: cursor_shape = IDC_ARROW; break;
			case CURSOR_POINTER: cursor_shape = IDC_HAND; break;
		}
		
		HCURSOR cursor = LoadCursorA(NULL, cursor_shape);
		
		window->curr_cursor_type = window->next_cursor_type;
		SetCursor(cursor);
	}
	
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
		{
			result.file_error = FILE_ERROR_GENERIC;
			printf("ERROR: %d\n", errno);
		}
		
		goto done_failure;
	}
	
	fseek(file, 0 , SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	s32 length_to_alloc = length+1;
	
	result.content = mem_alloc(length_to_alloc);
	if (!result.content) goto done;
	
	s32 read_result = fread(result.content, 1, length, file);
	if (read_result == 0 && length != 0)
	{
		mem_free(result.content);
		result.content = 0;
		return result;
	}
	
	result.content_length = read_result;
	
	((char*)result.content)[length] = 0;
	
	done:
	fclose(file);
	done_failure:
	return result;
}

bool platform_write_file_content(char *path, const char *mode, char *buffer, s32 len)
{
	bool result = false;
	
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

bool get_active_directory(char *buffer)
{
	return GetCurrentDirectory(MAX_INPUT_LENGTH, buffer);
}

bool set_active_directory(char *path)
{
	return SetCurrentDirectory(path);
}

static s32 filter_matches(array *filters, char *string, char **matched_filter)
{
	for (s32 i = 0; i < filters->length; i++)
	{
		char *filter = array_at(filters, i);
		if (string_match(filter, string))
		{
			*matched_filter = filter;
			return strlen(filter);
		}
	}
	return -1;
}

void platform_list_files_block(array *list, char *start_dir, array filters, bool recursive, memory_bucket *bucket,  bool include_directories, bool *is_cancelled)
{
	assert(list);
	s32 len = 0;
	char *matched_filter = 0;
	
	char *subdirname_buf;
	if (bucket)
		subdirname_buf = memory_bucket_reserve(bucket, MAX_INPUT_LENGTH);
	else
		subdirname_buf = mem_alloc(MAX_INPUT_LENGTH);
	
	char *start_dir_fix;
	if (bucket)
		start_dir_fix = memory_bucket_reserve(bucket, MAX_INPUT_LENGTH);
	else
		start_dir_fix = mem_alloc(MAX_INPUT_LENGTH);
	sprintf(start_dir_fix, "%s*", start_dir);
	
	char *start_dir_clean;
	if (bucket)
		start_dir_clean = memory_bucket_reserve(bucket, MAX_INPUT_LENGTH);
	else
		start_dir_clean = mem_alloc(MAX_INPUT_LENGTH);
	strncpy(start_dir_clean, start_dir, MAX_INPUT_LENGTH);
	
	WIN32_FIND_DATAA file_info;
	HWND handle = FindFirstFileA(start_dir_fix, &file_info);
	
	if (!bucket)
		mem_free(start_dir_fix);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
		return;
	}
	
	do
	{
		if (*is_cancelled) break;
		char *name = file_info.cFileName;
		
		// symbolic link is not allowed..
		if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
			continue;
		
		
		if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))
				continue;
			
			if (include_directories)
			{
				if ((len = filter_matches(&filters, name, 
										  &matched_filter)) && len != -1)
				{
					// is file
					char *buf;
					if (bucket)
						buf = memory_bucket_reserve(bucket, MAX_INPUT_LENGTH);
					else
						buf = mem_alloc(MAX_INPUT_LENGTH);
					sprintf(buf, "%s%s",start_dir, name);
					
					found_file f;
					f.path = buf;
					
					if (bucket)
						f.matched_filter= memory_bucket_reserve(bucket, len+1);
					else
						f.matched_filter= mem_alloc(len+1);
					
					strncpy(f.matched_filter, matched_filter, len+1);
					
					mutex_lock(&list->mutex);
					array_push_size(list, &f, sizeof(found_file));
					mutex_unlock(&list->mutex);
				}
			}
			
			if (recursive)
			{
				strncpy(subdirname_buf, start_dir_clean, MAX_INPUT_LENGTH);
				strcat(subdirname_buf, name);
				strcat(subdirname_buf, "\\");
				
				// is directory
				platform_list_files_block(list, subdirname_buf, filters, recursive, bucket, include_directories, is_cancelled);
			}
		}
		else if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) || 
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
		{
			if ((len = filter_matches(&filters, name, 
									  &matched_filter)) && len != -1)
			{
				// is file
				char *buf;
				if (bucket)
					buf = memory_bucket_reserve(bucket, MAX_INPUT_LENGTH);
				else
					buf = mem_alloc(MAX_INPUT_LENGTH);
				
				sprintf(buf, "%s%s",start_dir, name);
				
				found_file f;
				f.path = buf;
				
				if (bucket)
					f.matched_filter = memory_bucket_reserve(bucket, len+1);
				else
					f.matched_filter = mem_alloc(len+1);
				
				strncpy(f.matched_filter, matched_filter, len+1);
				
				mutex_lock(&list->mutex);
				array_push_size(list, &f, sizeof(found_file));
				mutex_unlock(&list->mutex);
			}
		}
	}
	while (FindNextFile(handle, &file_info) != 0);
	
	if (!bucket)
		mem_free(start_dir_clean);
	
	FindClose(handle);
}

static void* platform_open_file_dialog_implementation(void *data)
{
	struct open_dialog_args *args = data;
	
	OPENFILENAME info;
	info.lStructSize = sizeof(OPENFILENAME);
	info.hwndOwner = NULL;
	info.hInstance = NULL;
	
	if (args->file_filter)
	{
		char filter[50];
		strncpy(filter, args->file_filter, 50);
		filter[strlen(filter)+1] = 0;
		info.lpstrFilter = filter;
	}
	else
	{
		info.lpstrFilter = NULL;
	}
	
	char szFile[256 * MAX_PATH];
	char szPath[MAX_PATH];
	
	info.lpstrCustomFilter = NULL;
	info.nMaxCustFilter = 50;
	info.nFilterIndex = 0;
	info.lpstrFile = (char*)szFile;
	info.lpstrFile[0] = 0;
	info.nMaxFile = sizeof(szFile);
	
	info.lpstrFileTitle = NULL;
	info.lpstrInitialDir = args->start_path;
	info.lpstrTitle = NULL;
	
	if (args->type == SAVE_FILE)
	{
		info.Flags = OFN_PATHMUSTEXIST;
		GetSaveFileNameA(&info);
		strncpy(args->buffer, info.lpstrFile, MAX_INPUT_LENGTH);
	}
	else if (args->type == OPEN_DIRECTORY)
	{
		BROWSEINFOA inf;
		
		PIDLIST_ABSOLUTE result = SHBrowseForFolderA(&inf);
		
		if (!result) return 0;
		
		SHGetPathFromIDListA(result, args->buffer);
	}
	else if (args->type == OPEN_FILE)
	{
		info.Flags = OFN_FILEMUSTEXIST;
		GetOpenFileNameA(&info);
		strncpy(args->buffer, info.lpstrFile, MAX_INPUT_LENGTH);
	}
	
	
	return 0;
}

void *platform_open_file_dialog_block(void *arg)
{
	platform_open_file_dialog_implementation(arg);
	mem_free(arg);
	return 0;
}

char *platform_get_full_path(char *file)
{
	char *buf = mem_alloc(MAX_INPUT_LENGTH);
	if (!GetFullPathNameA(file, MAX_INPUT_LENGTH, buf, NULL))
	{
		buf[0] = 0;
	}
	
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

void platform_init(int argc, char **argv)
{
	setlocale(LC_ALL, "en_US.UTF-8");
	
	QueryPerformanceFrequency(&perf_frequency);
	CoInitialize(NULL);
	create_key_tables();
	
	instance = GetModuleHandle(NULL);
	cmd_show = argc;
	
	// get fullpath of the directory the exe is residing in
	binary_path = platform_get_full_path(argv[0]);
	
	platform_create_config_directory();
	
	// if program is run from a folder included in PATH
	//if (string_equals(binary_path, ""))
	//{
	//sprintf(binary_path, "%s", INSTALL_DIRECTORY);
	//}
	
	char buf[MAX_INPUT_LENGTH];
	get_directory_from_path(buf, binary_path);
	strncpy(binary_path, buf, MAX_INPUT_LENGTH-1);
	
	assets_create();
}

void platform_set_icon(platform_window *window, image *img)
{
	BYTE *bmp;
	s32 data_len = img->width * img->height * 4;
	s32 total_len = data_len + 40 * 4;
	
	bmp = mem_alloc(total_len);
	
	struct {
		int32_t header_size, width, geight;
		int16_t color_plane, bits_per_pixel;
		int32_t compression_mode, img_length, obsolete[4];
	} bmp_header = {40, img->width, img->height * 2, 1, 32, BI_RGB, data_len, {0,0,0,0} };
	
	memcpy(bmp, &bmp_header, 40);
	
	s32 index = 0;
	for (s32 y = img->height-1; y >= 0; y--)
	{
		for (s32 x = 0; x < img->width; x++)
		{
			s32 img_pixel = *(((s32*)img->data+(x+(y*img->width))));
			
			// 0xAABBGGRR
			s32 a = (img_pixel>>24) & 0x000000FF;
			s32 b = (img_pixel>>16) & 0x000000FF;
			s32 g = (img_pixel>> 8) & 0x000000FF;
			s32 r = (img_pixel>> 0) & 0x000000FF;
			
			//s32 c = (r << 24) | (g << 16) | (b << 8) | (a << 0);
			s32 c = (b << 24) | (r << 16) | (g << 8) | (a << 0);
			memcpy(bmp+40+(index*4), &c, 4);
			
			++index;
		}
	}
	
	HICON icon = CreateIconFromResource(bmp, total_len, TRUE, 0x00030000);
	
	SendMessage(window->window_handle, WM_SETICON, ICON_SMALL, (LPARAM)icon);
	SendMessage(window->window_handle, WM_SETICON, ICON_BIG, (LPARAM)icon);
	
	mem_free(bmp);
}

u64 platform_get_time(time_type time_type, time_precision precision)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	
	double sec = counter.QuadPart / (double)(perf_frequency.QuadPart);
	
	//printf("%I64d %I64d %f\n", counter.QuadPart, perf_frequency.QuadPart, sec);
	
	double val = sec;
	
	if (precision == TIME_NS)
	{
		return val*1000000000;
	}
	if (precision == TIME_US)
	{
		return val*1000000;
	}
	if (precision == TIME_MILI_S)
	{
		return val*1000;
	}
	if (precision == TIME_S)
	{
		return val;
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
	info.model = 1;
	
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