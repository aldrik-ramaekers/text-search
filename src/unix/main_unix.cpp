#include "imgui.h"
#include "imgui_spectrum.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../utf8.h"
#include "platform.h"
#include "mutex.h"
#include "array.h"
#include "memory_bucket.h"
#include "image.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

void ts_create_gui(int window_w, int window_h);
void ts_load_images();
void ts_init();

bool program_running = true;

char config_path[MAX_INPUT_LENGTH];
static const char* _ts_platform_get_config_file_path(char* buffer) {
	// TODO: this can be easily merged.
#ifdef __APPLE__
	char* env = getenv("HOME");
	char path_buf[MAX_INPUT_LENGTH];
	snprintf(path_buf, MAX_INPUT_LENGTH, "%s%s", env, "/Library/Application Support/text-search");
	snprintf(buffer, MAX_INPUT_LENGTH, "%s%s", path_buf, "/imgui.ini");

	if (!ts_platform_dir_exists(path_buf)) {
		
		mkdir(path_buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	return buffer;
#else
	char* env = getenv("HOME");
	char path_buf[MAX_INPUT_LENGTH];
	snprintf(path_buf, MAX_INPUT_LENGTH, "%s%s", env, "/text-search/");
	snprintf(buffer, MAX_INPUT_LENGTH, "%.*s%s", MAX_INPUT_LENGTH-10, path_buf, "imgui.ini");
	if (!ts_platform_dir_exists(path_buf)) {
		mkdir(path_buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	return buffer;
#endif
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Text-Search", nullptr, nullptr);
    if (window == nullptr) {
        return 1;
	}
	
	glfwSetWindowSizeLimits(window, 800, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
	io.IniFilename = _ts_platform_get_config_file_path(config_path);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup Dear ImGui style
    ImGui::Spectrum::StyleColorsSpectrum();
	ImGui::Spectrum::LoadFont(18.0f);

	ts_init();
	ts_load_images();
	ts_load_config();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	int display_w, display_h;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		ts_create_gui(display_w, display_h);

        // Rendering
        ImGui::Render();  
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

bool ts_platform_dir_exists(utf8_int8_t* path) {
	DIR* dir = opendir(path);
	if (dir) {
		/* Directory exists. */
		closedir(dir);
		return true;
	} else if (ENOENT == errno) {
		return false; // does not exist
	} else {
		return false; // error opening dir
	}
}

ts_file_content ts_platform_read_file(char *path, const char *mode) {
	ts_file_content result;
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
		else if (errno == ESTALE)
			result.file_error = FILE_ERROR_STALE;
		else
		{
			result.file_error = FILE_ERROR_GENERIC;
		}
		
		return result;
	}
	
	fseek(file, 0 , SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	int length_to_alloc = length+1;
	
	result.content = malloc(length_to_alloc);
	if (!result.content) {
		fclose(file);
		return result;
	}
	
	memset(result.content, 0, length);
	int read_result = fread(result.content, 1, length, file);
	if (read_result == 0 && length != 0)
	{
		free(result.content);
		result.content = 0;
		return result;
	}
	
	result.content_length = read_result;
	
	((char*)result.content)[length] = 0;
	
	fclose(file);
	return result;
}

void ts_platform_list_files_block(ts_search_result* result, wchar_t* start_dir) {

	utf8_int8_t* search_dir = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
	if (start_dir == nullptr) {
		strcpy(search_dir, result->directory_to_search);
	}
	else {
		strcpy(search_dir, (char*)start_dir);
	}

	// Append wildcard
	utf8_int8_t* search_dir_fix = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
    strcpy(search_dir_fix, search_dir);
    strcat(search_dir_fix, u8"/*");
	
	DIR *d;
	struct dirent *dir;
	d = opendir(search_dir);
	if (d) {
		if (chdir(search_dir) != 0) return;
		while ((dir = readdir(d)) != NULL) {
			if (result->cancel_search) return;
			if (chdir(search_dir) != 0) continue;
			
			if (dir->d_type == DT_DIR)
			{
				if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0))
					continue;
				
				utf8_int8_t complete_file_path[MAX_INPUT_LENGTH];
				strcpy(complete_file_path, search_dir);
				strcat(complete_file_path, "/");
				strcat(complete_file_path, dir->d_name);
				
				// do recursive search
				ts_platform_list_files_block(result, (wchar_t*)complete_file_path);
			}
			// we handle DT_UNKNOWN for file systems that do not support type lookup.
			else if (dir->d_type == DT_REG || dir->d_type == DT_UNKNOWN)
			{
				char *matched_filter = 0;
				if (ts_filter_matches(&result->filters, dir->d_name, &matched_filter) == (size_t)-1) {
					continue;
				}
				(void)matched_filter;

				
				utf8_int8_t complete_file_path[MAX_INPUT_LENGTH];
				strcpy(complete_file_path, search_dir);
				strcat(complete_file_path, "/");
				strcat(complete_file_path, dir->d_name);
				
				ts_found_file* f = (ts_found_file*)ts_memory_bucket_reserve(&result->memory, sizeof(ts_found_file));
				f->path = (utf8_int8_t*)ts_memory_bucket_reserve(&result->memory, MAX_INPUT_LENGTH);
				f->match_count = 0;
				f->error = 0;
				f->collapsed = false;
				strcpy(f->path, complete_file_path);
				
				ts_mutex_lock(&result->files.mutex);
				ts_array_push_size(&result->files, &f, sizeof(ts_found_file*));
				ts_mutex_unlock(&result->files.mutex);
			}
		}
		closedir(d);
	}
}

uint64_t ts_platform_get_time(uint64_t compare) {
	struct timespec tms;
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tms)) {
		return -1;
	}
	uint64_t result = 0;
	result = tms.tv_sec * 1000;
	result += tms.tv_nsec / 1000000;

	if (compare != 0) {
		return (result - compare);
	}

	return result;
}

void ts_platform_open_file_as(utf8_int8_t* str) {
	// not implemented
}

void ts_platform_open_file_in_folder(utf8_int8_t* file) {
	// not implemented
}