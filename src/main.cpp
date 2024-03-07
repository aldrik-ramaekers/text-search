#include "../imgui/imgui.h"
#include "../imgui/imgui_spectrum.h"
#include "../imgui/imgui_impl_opengl3_loader.h"
#include "../imspinner/imspinner.h"
#include "../imfiledialog/imFileDialog.h"
#include "../utf8.h"
#include "definitions.h"
#include "search.h"
#include "platform.h"
#include "image.h"
#include "config.h"

#include <stdio.h>

// Popups
bool open_settings_window = false;
bool open_about_window = false;

char* help_text = 
				"1. Search directory\n"
				"	- The absolute path to the folder to search.\n"
				"2. File filter\n"
				"	- Filter files that should be included in file search\n"
				"	- Multiple filters can be declared separated by comma ','\n"
				"	- Supports wildcards '*' & '?' in filter\n"
				"3. Text to search\n"
				"	- Supports wildcards '*' & '?' in text\n";

static void _ts_create_popups() {
	ImGuiIO& io = ImGui::GetIO();
	if (open_settings_window) {
		ImGui::OpenPopup("Text-Search settings");
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	}

	// Settings window
	if (ImGui::BeginPopupModal("Text-Search settings", &open_settings_window, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
		ImGui::SetWindowSize({300, 0});
		ImGui::DragInt("Threads", &ts_thread_count, 0.1f, 1, 8);
		ImGui::SetItemTooltip("Number of threads used to search for text matches");

		ImGui::DragInt("File Size", &max_file_size, 50.0f, 1, 10000, "%dMB");
		ImGui::SetItemTooltip("Files larger than this will not be searched");

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
	if (ImGui::BeginPopupModal("About Text-Search", &open_about_window, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
		ImGui::SetWindowSize({600, 420});

		char* name = "Text-Search";
		char* link = "created by Aldrik Ramaekers <aldrik.ramaekers@gmail.com>";

		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 64) / 2.0f);
		ImGui::Image((void*)(intptr_t)img_logo.id, {64, 64});
		ImGui::Dummy({0, 20});

		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(name).x)/2.0f);
		ImGui::Text(name);
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(link).x)/2.0f);
		ImGui::Text(link);
		ImGui::Dummy({0, 20});
		
		if (ImGui::CollapsingHeader("License")) {
			char* license = (char*)_binary_LICENSE_start;
			int license_length = _binary_LICENSE_end - _binary_LICENSE_start;
			ImGui::Text("%.*s", license_length, license);
		}

		ImGui::SeparatorText("Dependencies");
		{
			if (ImGui::TreeNode("https://github.com/ocornut/imgui")) {
				char* license = (char*)_binary_imgui_LICENSE_start;
				int license_length = _binary_imgui_LICENSE_end - _binary_imgui_LICENSE_start;
				ImGui::Text("%.*s", license_length, license);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("https://github.com/dalerank/imspinner")) {
				char* license = (char*)_binary_imspinner_LICENSE_start;
				int license_length = _binary_imspinner_LICENSE_end - _binary_imspinner_LICENSE_start;
				ImGui::Text("%.*s", license_length, license);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("https://github.com/dfranx/ImFileDialog")) {
				char* license = (char*)_binary_imfiledialog_LICENSE_start;
				int license_length = _binary_imfiledialog_LICENSE_end - _binary_imfiledialog_LICENSE_start;
				ImGui::Text("%.*s", license_length, license);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("https://github.com/nothings/stb/blob/master/stb_image.h")) {
				ImGui::Text("public domain");
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("https://github.com/sheredom/utf8.h")) {
				ImGui::Text("public domain");
				ImGui::TreePop();
			}		
		}

		ImGui::Dummy({0, 10});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Button("Close")) {
			open_about_window = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(TS_VERSION).x - 25);
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
			//ImGui::MenuItem("Open", "CTRL+O");
			//ImGui::MenuItem("Save", "CTRL+S");
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) {
				program_running = false;
			}
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

void ts_init() {
	// idk
}

int _tb_query_input_cb(ImGuiInputTextCallbackData* data) {
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		utf8ncpy(query_buffer, data->Buf, MAX_INPUT_LENGTH);
		ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size);
	}

	return 0;
}

void _ts_create_file_match_rows() {
	int itemcount = current_search_result == 0 ? 0 : current_search_result->files.length;
	for (int item = 0; item < itemcount; item++)
	{
		ts_found_file *file = *(ts_found_file **)ts_array_at(&current_search_result->files, item);

		char match_info_txt[20];
		snprintf(match_info_txt, 20, "#%d", item+1);
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableHeader(match_info_txt);

		ImGui::TableNextColumn();
		ImGui::TableHeader(file->path);

		ImGui::TableNextColumn();	
		ImGui::TableHeader("");
	}
}

utf8_int8_t* _ts_file_error_to_message(ts_file_open_error err) {
	switch (err) {
		case FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS: return u8"Too many open files";
		case FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM: return u8"Too many open files";
		case FILE_ERROR_NO_ACCESS: return u8"No permissions";
		case FILE_ERROR_NOT_FOUND: return u8"File not found";
		case FILE_ERROR_CONNECTION_ABORTED: return u8"Connection aborted";
		case FILE_ERROR_CONNECTION_REFUSED: return u8"Failed to connect";
		case FILE_ERROR_NETWORK_DOWN: return u8"Drive disconnected";
		case FILE_ERROR_REMOTE_IO_ERROR: return u8"Remote IO error";
		case FILE_ERROR_STALE: return u8"Server file moved";
		case FILE_ERROR_GENERIC: return u8"Failed to open file";
		case FILE_ERROR_TOO_BIG: return u8"File too big";
	}
	return "";
}

void _ts_create_file_error_rows() {
	int itemcount = current_search_result == 0 ? 0 : current_search_result->files.length;
	for (int item = 0; item < itemcount; item++)
	{
		ts_found_file *file = *(ts_found_file **)ts_array_at(&current_search_result->files, item);
		if (file->error == FILE_ERROR_NONE) continue;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::Spectrum::Color(0xFF0000));
		ImGui::TableHeader("ERROR");
		ImGui::PopStyleColor();

		ImGui::TableNextColumn();
		ImGui::TableHeader(file->path);

		ImGui::TableNextColumn();	
		ImGui::TableHeader(_ts_file_error_to_message((ts_file_open_error)file->error));
	}
}

void _ts_create_text_match_rows() {
	int itemcount = current_search_result == 0 ? 0 : current_search_result->matches.length;
	ts_found_file* prev_file = nullptr;
	for (int item = 0; item < itemcount; item++)
	{
		ts_file_match *file = (ts_file_match *)ts_array_at(&current_search_result->matches, item);

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
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::TextColored({255,0,0,255}, "%.*s", file->word_match_length, file->line_info + file->word_match_offset); 
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::TextUnformatted(file->line_info + file->word_match_offset + file->word_match_length);	
		
		ImGui::TableNextColumn();
		ImGui::Text("line %d", file->line_nr);	
	}
	_ts_create_file_error_rows();
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

			bool dir_exists = ts_platform_dir_exists(path_buffer);
			if (!dir_exists) ImGui::PushStyleColor(ImGuiCol_Border, ImGui::Spectrum::Color(0xCC2222));
			ImGui::InputTextWithHint("path-ti", "Path", path_buffer, MAX_INPUT_LENGTH);
			if (!dir_exists) ImGui::PopStyleColor();
			ImGui::PopItemWidth();
			ImGui::SetItemTooltip("Absolute path to directory to search");

			ImGui::PushItemWidth(-1);
			if (ImGui::InputTextWithHint("query", "Query", query_buffer, MAX_INPUT_LENGTH, ImGuiInputTextFlags_CallbackEdit|ImGuiInputTextFlags_EnterReturnsTrue, _tb_query_input_cb)) {
				ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size);
			}
			ImGui::PopItemWidth();
			ImGui::SetItemTooltip("Text to search within files, supports '*' & '?' wildcards");
			ImGui::PopStyleVar();
		}
		ImGui::EndChild();

		ImGui::SetNextWindowPos({offset + frame_w + separator_w, pos_y});
		ImGui::BeginChild("search-boxes2", ImVec2(frame_w, textbox_area_height), false);
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			if (ImGui::ImageButton("Folder", (void*)(intptr_t)img_folder.id, ImVec2(18.0f, 18.0f))) {
				ifd::FileDialog::Instance().Open("FolderSelectDialog", "Select a directory", "");
			}
			if (ifd::FileDialog::Instance().IsDone("FolderSelectDialog", window_w, window_h)) {
				if (ifd::FileDialog::Instance().HasResult()) {
					std::string res = ifd::FileDialog::Instance().GetResult().u8string();
					snprintf(path_buffer, MAX_INPUT_LENGTH, res.c_str());
				}
				ifd::FileDialog::Instance().Close();
			}

			ImGui::SameLine();
			ImGui::PushItemWidth(-1);
			if (ImGui::InputTextWithHint("filter-ti", "Filter", filter_buffer, MAX_INPUT_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue)) {
				ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size);
			}
			ImGui::PopItemWidth();
			ImGui::SetItemTooltip("Files to filter, supports '*' & '?' wildcards");
			ImGui::PopStyleVar();

			if (current_search_result && !current_search_result->search_completed) {
				ImSpinner::SpinnerIncScaleDots("Spinner", 10.0f, 2.0f, ImColor(70,70,70), 5.0f);
			}
			else {
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				if (ImGui::ImageButton("Search", (void*)(intptr_t)img_search.id, ImVec2(18.0f, 18.0f))) {
					ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size);
				}
				ImGui::PopStyleVar();
			}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();
	}
	pos_y += textbox_area_height + 7;

	if (current_search_result)
	{ // Results
		ImGui::SetNextWindowPos({5, pos_y});

		if (ImGui::BeginTable("results-table", 3, ImGuiTableFlags_BordersH|ImGuiTableFlags_ScrollY|ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingFixedFit,
			{(float)window_w-7.0f, (float)result_area_height}))
		{
			int nr_w = 50;
			int line_w = 180;
			int file_w = ImGui::GetWindowWidth() - line_w - nr_w;
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoHeaderLabel, nr_w);
			ImGui::TableSetupColumn("File", 0, file_w);
			ImGui::TableSetupColumn("Match", 0, line_w);			
			ImGui::TableHeadersRow();

			if (current_search_result->search_text == nullptr) _ts_create_file_match_rows();
			else _ts_create_text_match_rows();

			if (current_search_result->search_completed && (current_search_result->files.length == 0 || current_search_result->match_count == 0)) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("");			
				ImGui::TableNextColumn();

				char* msg = "No matches found.";
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(msg).x)/2.0f);
				ImGui::TextWrapped(msg);
			}		
		
			ImGui::EndTable();
		}
	}
	else { // Help text
		ImGui::Separator();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
		ImGui::TextWrapped(help_text);
	}
	pos_y += result_area_height;

	{ // Statusbar
		ImGui::SetNextWindowPos({0, pos_y});

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
		ImGui::BeginChild("search-statusbar", ImVec2(window_w, statusbar_area_height), ImGuiChildFlags_None, ImGuiWindowFlags_None);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 7.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
		if (current_search_result) {
			if (current_search_result->search_text) ImGui::Text("Found %d matches in %d files", current_search_result->match_count, current_search_result->file_count);
			else ImGui::Text("Found %d files", current_search_result->files.length);
		}
		else ImGui::Text("No search completed");

		ImGui::SameLine();

		if (current_search_result && current_search_result->search_completed) {
			ImGui::SetCursorPosX(window_w - 10.0f - ImGui::CalcTextSize("999.999s elapsed").x);
			ImGui::Text("%.3fs elapsed", current_search_result->timestamp/1000.0f);
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::PopStyleVar();

	ImGui::End();
}