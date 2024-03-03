#include "../imgui/imgui.h"
#include "../imgui/imgui_spectrum.h"
#include "../imgui/imgui_impl_opengl3_loader.h"
#include "definitions.h"
#include "search.h"
#include "platform.h"
#include "../utf8.h"
#include <stdio.h>

typedef struct t_ts_image {
	GLuint id;
	int width;
	int height;
} ts_image;

ts_image img_logo;

#define SEARCH_BUFFER_SIZE 2048

search_result* current_search_result = nullptr;

char path_buffer[SEARCH_BUFFER_SIZE];
char filter_buffer[SEARCH_BUFFER_SIZE];
char query_buffer[SEARCH_BUFFER_SIZE];

bool open_settings_window = false;
bool open_about_window = false;

int thread_count = 4;
int current_locale_index = 0;
int locales_count = 2;
char* locales[] = {
	"English",
	"Dutch"
};

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTexture(unsigned char* data, unsigned long size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory(data, size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) {
		printf("Failed to load %s\n", stbi_failure_reason());
        return false;
	}

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

static ts_image _ts_load_image(unsigned char* data, unsigned long size) {
	int w = 0;
	int h = 0;
	GLuint id = 0;
	bool ret = LoadTexture(data, size, &id, &w, &h);

	return ts_image {id, w, h};
}

static void _ts_search_file(found_file* ref, file_content content, search_result* result) {
	if (content.content && !content.file_error)
	{
		array text_matches = array_create(sizeof(text_match));
		int search_len = strlen(result->search_text);
		if (string_contains_ex((char*)content.content, result->search_text, &text_matches))
		{
			mutex_lock(&result->matches.mutex);
			for (int i = 0; i < text_matches.length; i++)
			{
				text_match *m = (text_match *)array_at(&text_matches, i);

				file_match file_match;
				file_match.file = ref;
				file_match.line_nr = m->line_nr;
				file_match.word_match_offset = m->word_offset;
				file_match.word_match_length = m->word_match_len;
				file_match.line_info = (char*)malloc(MAX_INPUT_LENGTH);

				int text_pad_lr = 25;
				if (file_match.word_match_offset > text_pad_lr) {
					m->line_start += file_match.word_match_offset - text_pad_lr;
					file_match.word_match_offset = text_pad_lr;
				}
				int total_len = text_pad_lr + search_len + text_pad_lr;

				snprintf(file_match.line_info, MAX_INPUT_LENGTH, "%.*s", total_len, m->line_start);
				for (int i = 0; i < total_len; i++) {
					if (file_match.line_info[i] == '\n') file_match.line_info[i] = ' ';
					if (file_match.line_info[i] == '\t') file_match.line_info[i] = ' ';
					if (file_match.line_info[i] == '\r') file_match.line_info[i] = ' ';
					if (file_match.line_info[i] == '\x0B') file_match.line_info[i] = ' ';
				}
				
				array_push_size(&result->matches, &file_match, sizeof(file_match));
				ref->match_count++;
				result->match_count = result->matches.length;
			}
			mutex_unlock(&result->matches.mutex);
		}
		
		array_destroy(&text_matches);
	}
}

static void* _ts_search_thread(void* args) {
	search_result* new_result = (search_result *)args;

	keep_going:;
	while (new_result->file_list_read_cursor < new_result->files.length)
	{
		mutex_lock(&new_result->files.mutex);
		int read_cursor = new_result->file_list_read_cursor++;
		new_result->file_count++;
		mutex_unlock(&new_result->files.mutex);

		if (read_cursor >= new_result->files.length) continue;

		found_file* f = (found_file*)array_at(&new_result->files, read_cursor);
		file_content content = platform_read_file(f->path, "rb");

		_ts_search_file(f, content, new_result);

		free(content.content);
	}

	if (!new_result->done_finding_files)
		goto keep_going;

	new_result->completed_match_threads++;

	return 0;
}

static void _ts_start_search() {
	search_result* new_result = create_empty_search_result();
	snprintf(new_result->directory_to_search, MAX_INPUT_LENGTH, "%s", path_buffer);
	snprintf(new_result->search_text, MAX_INPUT_LENGTH, "%s", query_buffer);
	

	platform_list_files(new_result);
	//new_result->max_thread_count
	for (int i = 0; i < 1; i++) {
		thread thr = thread_start(_ts_search_thread, new_result);
		thread_detach(&thr);
	}

	current_search_result = new_result;
}

static void _ts_create_popups() {
	ImGuiIO& io = ImGui::GetIO();
	if (open_settings_window) {
		ImGui::OpenPopup("Text-Search settings");
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	}

	// Settings window
	if (ImGui::BeginPopupModal("Text-Search settings", NULL, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
		ImGui::SetWindowSize({300, 0});
		ImGui::DragInt("Threads", &thread_count, 1.0f, 1, 64);
		ImGui::Combo("Language", &current_locale_index, locales, locales_count);

		ImGui::Dummy({0, 70});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Button("Close")) {
			open_settings_window = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	if (open_about_window) {
		ImGui::OpenPopup("About Text-Search");
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	}

	// About window
	if (ImGui::BeginPopupModal("About Text-Search", NULL, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
		ImGui::SetWindowSize({600, 0});

		//ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 64 - 10);
		//ImGui::Image((void*)(intptr_t)img_logo.id, {64, 64});
		
		char* license = (char*)_binary_LICENSE_start;
		int license_length = _binary_LICENSE_end - _binary_LICENSE_start;
		ImGui::Text("%.*s", license_length, license);

		ImGui::Dummy({0, 70});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Button("Close")) {
			open_about_window = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(TS_VERSION).x - 15);
		ImGui::Text(TS_VERSION);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}
}

static int _ts_create_menu() {
	int menu_bar_h = 0;
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::Spectrum::Color(0xDDDDDD));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("Open", "CTRL+O");
			ImGui::MenuItem("Save", "CTRL+S");
			ImGui::Separator();
			ImGui::MenuItem("Exit", "CTRL+Q");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Program"))
		{
			if (ImGui::MenuItem("Settings")) {
				open_settings_window = true;
			}
			if (ImGui::MenuItem("About")) {
				open_about_window = true;
			}
			
			ImGui::EndMenu();
		}

		menu_bar_h = 27;
		ImGui::EndMenuBar();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	_ts_create_popups();

	return menu_bar_h;
}

void ts_load_images() {
	snprintf(path_buffer, MAX_INPUT_LENGTH, "%s", "C:\\Users\\aldri\\Desktop\\Vault\\Projects\\allegro5");
	snprintf(filter_buffer, MAX_INPUT_LENGTH, "%s", "*.h");
	snprintf(query_buffer, MAX_INPUT_LENGTH, "%s", "test");

	int size = _binary_misc_logo_64_png_end - _binary_misc_logo_64_png_start;
	unsigned char* data = (unsigned char *)_binary_misc_logo_64_png_start;
	img_logo = _ts_load_image(data, size);
}

void ts_create_gui(int window_w, int window_h) {
	static float f = 0.0f;
	static int counter = 0;
	int window_pad = 50;
	int textbox_area_height = 80;
	int statusbar_area_height = 30;
	int result_area_height = window_h - textbox_area_height - statusbar_area_height - window_pad;

	ImGui::SetNextWindowSize({(float)window_w, (float)window_h});
	ImGui::SetNextWindowPos({0, 0});

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));
	ImGui::Begin("text-search", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar|
		ImGuiWindowFlags_MenuBar);
	ImGui::PopStyleVar();

	float menu_bar_h = _ts_create_menu();

	float pos_y = 0;

	pos_y += menu_bar_h + 15;

	{ // Search boxes
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

		float offset = 15.0f;
		float separator_w = 10.0f;
		float frame_w = window_w/2.5f - offset - separator_w/2.0f;
		ImGui::SetNextWindowPos({offset, pos_y});
		ImGui::BeginChild("search-boxes", ImVec2(frame_w, textbox_area_height), false);
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushItemWidth(-1);
			ImGui::InputTextWithHint("path-ti", "Path", path_buffer, 4000);
			ImGui::PopItemWidth();		

			ImGui::PushItemWidth(-1);
			ImGui::InputTextWithHint("query", "Query", query_buffer, 4000);
			ImGui::PopItemWidth();
			ImGui::PopStyleVar();
		}
		ImGui::EndChild();

		ImGui::SetNextWindowPos({offset + frame_w + separator_w, pos_y});
		ImGui::BeginChild("search-boxes2", ImVec2(frame_w, textbox_area_height), false);
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushItemWidth(-1);
			ImGui::InputTextWithHint("filter-ti", "Filter", filter_buffer, 4000);
			ImGui::PopItemWidth();
			ImGui::PopStyleVar();

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			if (ImGui::Button("Search")) {
				_ts_start_search();
			}
			ImGui::PopStyleVar();
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();
	}
	pos_y += textbox_area_height + 7;

	{ // Results
		ImGui::SetNextWindowPos({5, pos_y});

		if (ImGui::BeginTable("results-table", 3, ImGuiTableFlags_BordersH|ImGuiTableFlags_ScrollY|ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingFixedFit,
			{(float)window_w-7.0f, (float)result_area_height}))
		{
			int nr_w = 50;
			int line_w = 120;
			int file_w = ImGui::GetWindowWidth() - line_w - nr_w;
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoHeaderLabel, nr_w);
			ImGui::TableSetupColumn("File", 0, file_w);
			ImGui::TableSetupColumn("Match", 0, line_w);			
			ImGui::TableHeadersRow();

			int itemcount = current_search_result == 0 ? 0 : current_search_result->matches.length;
			found_file* prev_file = nullptr;
			for (int item = 0; item < itemcount; item++)
			{
				file_match *file = (file_match *)array_at(&current_search_result->matches, item);

				if (prev_file != file->file) {
					prev_file = file->file;
					char match_info_txt[20];

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TableHeader("");

					ImGui::TableNextColumn();
					ImGui::TableHeader(file->file->path);

					ImGui::TableNextColumn();	
					snprintf(match_info_txt, 20, "%d match(es)", file->file->match_count);
					ImGui::TableHeader(match_info_txt);
				}

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("#%d", item+1);
							
				ImGui::TableNextColumn();
				ImGui::Text("%.*s", file->word_match_offset, file->line_info);
				ImGui::SameLine();
				ImGui::TextColored({255,0,0,255}, "%.*s", file->word_match_length, file->line_info + file->word_match_offset); 
				ImGui::SameLine();
				ImGui::TextUnformatted(file->line_info + file->word_match_offset + file->word_match_length);	
				
				ImGui::TableNextColumn();
				ImGui::Text("line %d", file->line_nr);	
			}
			ImGui::EndTable();
		}
	}
	pos_y += result_area_height;

	{ // Statusbar
		ImGui::SetNextWindowPos({0, pos_y});

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
		ImGui::BeginChild("search-statusbar", ImVec2(window_w, statusbar_area_height), ImGuiChildFlags_None, ImGuiWindowFlags_None);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 7.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
		if (current_search_result) ImGui::Text("Found %d matches in %d files", current_search_result->match_count, current_search_result->file_count);
		else ImGui::Text("No search completed");

		ImGui::SameLine();

		ImGui::SetCursorPosX(window_w - 10.0f - ImGui::CalcTextSize("no search completed").x);
		ImGui::Text("no search completed");
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::PopStyleVar();

	ImGui::End();
}