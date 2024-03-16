#include <stdio.h>

#include "imgui.h"
#include "imgui_spectrum.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#include "../utf8.h"
#include "platform.h"
#include "mutex.h"
#include "array.h"
#include "memory_bucket.h"
#include "image.h"
#include "config.h"
#include "fonts.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _CRT_SECURE_NO_WARNINGS
#include <pathcch.h>
#include <windows.h>
#include <Shlobj.h>
#include <GL/GL.h>
#include <tchar.h>
#include "DropManager.h"

#ifdef TS_RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#define IDI_LOGO 123

char config_path[MAX_INPUT_LENGTH];
ts_dragdrop_data dragdrop_data = {0};

void ts_create_gui(int window_w, int window_h);
void ts_load_images();
void ts_init();

struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;
LARGE_INTEGER 			Frequency;
bool 					program_running = true;
HWND 					window_handle;

bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const char* _ts_platform_get_config_file_path(char* buffer) {
	if(SUCCEEDED(SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, buffer)))
	{
		strcat_s(buffer, MAX_INPUT_LENGTH, "\\text-search\\config.ini");
		return buffer;
	}
	
	return 0;
}

bool ts_platform_dir_exists(utf8_int8_t* dir)
{
  DWORD ftyp = GetFileAttributesA(dir);
  if (ftyp == INVALID_FILE_ATTRIBUTES)
    return false;
  if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
    return true;
  return false;
}

uint64_t ts_platform_get_time(uint64_t compare)
{
	LARGE_INTEGER stamp;
	QueryPerformanceCounter(&stamp);
	if (compare != 0) {
		return (stamp.QuadPart - compare) * 1000 / Frequency.QuadPart;
	}
	return stamp.QuadPart;
}

static void _ts_platform_draw_frame() {
	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ts_create_gui(g_Width, g_Height);

	// Rendering
	ImGui::Render();
	glViewport(0, 0, g_Width, g_Height);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Present
	::SwapBuffers(g_MainWindow.hDC);
}

void ts_platform_set_window_title(utf8_int8_t* str) {
	// convert utf8 to wchar path.
	wchar_t wchar_buffer[MAX_INPUT_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, (wchar_t*)wchar_buffer, MAX_INPUT_LENGTH);
	
	SetWindowTextW(window_handle, wchar_buffer);
}

ts_font_range _ts_get_FONT_RANGE_to_load() {
	wchar_t buffer[50];
	GetUserDefaultLocaleName(buffer, 50);

	ts_font_range result = FONT_RANGE_ENGLISH;
	if (wcscmp(buffer, L"el-GR") == 0) result = FONT_RANGE_GREEK;
	if (wcscmp(buffer, L"ko-KR") == 0) result = FONT_RANGE_KOREAN;
	if (wcscmp(buffer, L"ja-JP") == 0) result = FONT_RANGE_JAPANESE;

	if (wcscmp(buffer, L"be-BY") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"bg-BG") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ru-RU") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ru-MD") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ro-MD") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"kk-KZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"tt-RU") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ky-KG") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"mn-MN") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"az-Cyrl-AZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"uz-Cyrl-UZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"sr-Cyrl-CS") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"sr-Latn-CS") == 0) result = FONT_RANGE_CYRILLIC;

	if (wcscmp(buffer, L"bo-CN") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"zh-CN") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"mn-Mong-CN") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"zh-HK") == 0) result = FONT_RANGE_CHINESE_FULL;
	if (wcscmp(buffer, L"zh-TW") == 0) result = FONT_RANGE_CHINESE_FULL;
	if (wcscmp(buffer, L"zh-SG") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"zh-MO") == 0) result = FONT_RANGE_CHINESE_FULL;

	if (wcscmp(buffer, L"th-TH") == 0) result = FONT_RANGE_THAI;
	if (wcscmp(buffer, L"vi-VN") == 0) result = FONT_RANGE_VIETNAMESE;

	return result;
}

int main(int, char**)
{
	if (OleInitialize(NULL) != S_OK) {
		return -1;
	}

	ts_init();

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Text-Search", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
	window_handle = hwnd;

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

	HICON hicoCaption = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LOGO));
	SendMessage(hwnd, WM_SETICON, ICON_BIG,
				reinterpret_cast<LPARAM>(hicoCaption));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
	io.IniFilename = _ts_platform_get_config_file_path(config_path);

    ImGui::Spectrum::StyleColorsSpectrum();

    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

	QueryPerformanceFrequency(&Frequency);

	ts_load_fonts(18.0f, _ts_get_FONT_RANGE_to_load());
	ts_load_images();
	ts_load_config();

	DropManager dm;
    RegisterDragDrop(hwnd, &dm);

    while (program_running)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                program_running = false;
        }
        if (!program_running)
            break;

		_ts_platform_draw_frame();
    }

	ts_destroy_result(current_search_result);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

	RevokeDragDrop(hwnd);

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	OleUninitialize();
	CoUninitialize();

    return 0;
}

bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(NULL, NULL);
    ::ReleaseDC(hWnd, data->hDC);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool moveResizeLoop;

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
		case WM_GETMINMAXINFO: {
			MINMAXINFO *minmax = (MINMAXINFO *)lParam;
			minmax->ptMinTrackSize.x = 800;
			minmax->ptMinTrackSize.y = 600;
			break;
		}
		case WM_SIZE: {
			if (wParam != SIZE_MINIMIZED)
			{
				g_Width = LOWORD(lParam);
				g_Height = HIWORD(lParam);
			}
			return 0;
		}
		case WM_SYSCOMMAND: {
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
		} break;
		case WM_DESTROY: {
			::PostQuitMessage(0);
			return 0;
		} break;
		case WM_ENTERSIZEMOVE:
		case WM_ENTERMENULOOP: {
			SetTimer(hWnd, (UINT_PTR)&moveResizeLoop, USER_TIMER_MINIMUM, NULL);
		} break;
		case WM_TIMER: {
			if (wParam == (UINT_PTR)&moveResizeLoop) {
				_ts_platform_draw_frame();
				return 0;
			}
		} break;
		case WM_EXITSIZEMOVE:
		case WM_EXITMENULOOP: {
			KillTimer(hWnd, (UINT_PTR)&moveResizeLoop);
		} break;
	}
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

ts_file_content ts_platform_read_file(char *path, const char *mode)
{
	ts_file_content result;
	result.content = 0;
	result.content_length = 0;
	result.file_error = 0;

	// convert utf8 to wchar path.
	wchar_t wchar_buffer[MAX_INPUT_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, path, -1, (wchar_t*)wchar_buffer, MAX_INPUT_LENGTH);
	
	const size_t cSize = strlen(mode)+1;
    wchar_t* wc = new wchar_t[cSize];
	size_t outSize;
    mbstowcs_s(&outSize, wc, cSize, mode, cSize);

	FILE *file;
	_wfopen_s(&file, wchar_buffer, wc);
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
		}
		
		goto done_failure;
	}
	
	fseek(file, 0 , SEEK_END);
	long length = ftell(file);
	if (length == -1L) goto done_failure;
	fseek(file, 0, SEEK_SET);
	
	int length_to_alloc = length+1;
	
	result.content = malloc(length_to_alloc);
	if (!result.content) goto done;
	
	memset(result.content, 0, length_to_alloc);
	size_t read_result = fread(result.content, 1, (size_t)length, file);
	if (read_result == 0 && length != 0)
	{
		free(result.content);
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

void ts_platform_list_files_block(ts_search_result* result, wchar_t* start_dir)
{
    // Utf8 to wchar str
	wchar_t* search_dir = (wchar_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
	if (start_dir == NULL) {
		MultiByteToWideChar(CP_UTF8, 0, result->directory_to_search, -1, search_dir, MAX_INPUT_LENGTH);
	}
	else {
		wcscpy_s(search_dir, MAX_INPUT_LENGTH, start_dir);
	}

	// Append wildcard
	wchar_t* search_dir_fix = (wchar_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
    wcscpy_s(search_dir_fix, MAX_INPUT_LENGTH, search_dir);
    wcscat_s(search_dir_fix, MAX_INPUT_LENGTH, L"\\*");

	WIN32_FIND_DATAW file_info;
	HANDLE handle = FindFirstFileW(search_dir_fix, &file_info);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
		return;
	}
	
	do
	{
		if (result->cancel_search) return;

		//if (*is_cancelled) break;
		const wchar_t *name = (const wchar_t *)file_info.cFileName;
		
		// symbolic link is not allowed..
		if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
			continue;
		
		
		if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if ((wcscmp(name, L".") == 0) || (wcscmp(name, L"..") == 0))
				continue;
			
			wchar_t* subdir_buffer_path = (wchar_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
			wcscpy_s(subdir_buffer_path, MAX_INPUT_LENGTH, search_dir);
			wcscat_s(subdir_buffer_path, MAX_INPUT_LENGTH, L"\\");
			wcscat_s(subdir_buffer_path, MAX_INPUT_LENGTH, name);
			ts_platform_list_files_block(result, subdir_buffer_path);
		}
		else if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) || 
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
				 (file_info.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
		{
			char *matched_filter = 0;
			utf8_int8_t* uni_name = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
			WideCharToMultiByte(CP_UTF8,0,name,-1,(LPSTR)uni_name,MAX_INPUT_LENGTH, NULL, NULL);
			if (ts_filter_matches(&result->filters, uni_name, &matched_filter) == (size_t)-1) {
				continue;
			}
			(void)matched_filter;

			wchar_t* complete_file_path = (wchar_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
			wcscpy_s(complete_file_path, MAX_INPUT_LENGTH, search_dir);
			wcscat_s(complete_file_path, MAX_INPUT_LENGTH, L"\\");
			wcscat_s(complete_file_path, MAX_INPUT_LENGTH, name);

			ts_found_file* f = (ts_found_file*)ts_memory_bucket_reserve(&result->memory, sizeof(ts_found_file));
			f->path = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
			f->match_count = 0;
			f->error = 0;
			f->collapsed = false;
			f->file_size = (file_info.nFileSizeHigh * (MAXDWORD+1)) + file_info.nFileSizeLow;
			WideCharToMultiByte(CP_UTF8,0,complete_file_path,-1,(LPSTR)f->path,MAX_INPUT_LENGTH, NULL, NULL);
				
			ts_mutex_lock(&result->files.mutex);
			ts_array_push_size(&result->files, &f, sizeof(ts_found_file*));
			ts_mutex_unlock(&result->files.mutex);		
		}
	}
	while (FindNextFile(handle, (LPWIN32_FIND_DATAW)&file_info) != 0);
	
	FindClose(handle);
}

void ts_platform_open_file_as(utf8_int8_t* str) {
	OPENASINFO info;

	wchar_t convstr[MAX_INPUT_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, convstr, MAX_INPUT_LENGTH);

	info.pcszFile = convstr;
	info.pcszClass = NULL;
	info.oaifInFlags = OAIF_EXEC;

	SHOpenWithDialog(NULL, &info);
}

void ts_platform_open_file_in_folder(utf8_int8_t* file) {

	wchar_t convstr[MAX_INPUT_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, file, -1, convstr, MAX_INPUT_LENGTH);
	PathCchRemoveFileSpec(convstr, MAX_INPUT_LENGTH);
	ShellExecuteW(NULL, L"open", convstr, NULL, NULL, SW_SHOWDEFAULT);
}