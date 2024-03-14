#include "../imgui/imgui.h"
#include "../imgui/imgui_spectrum.h"
#include "../imgui/imgui_impl_opengl3_loader.h"
#include "../imfiledialog/ImFileDialog.h"
#include "../utf8.h"
#include "widgets/imgui_toggle.h"
#include "definitions.h"
#include "search.h"
#include "platform.h"
#include "image.h"
#include "config.h"
#include "export.h"
#include "import.h"

#include <stdio.h>

// Popups
bool open_settings_window = false;
bool open_about_window = false;
export_result last_export_result = EXPORT_NONE;

const char* help_text = 
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
		ImGui::DragInt("Threads", (int*)&ts_thread_count, 0.1f, 1, 8);
		ImGui::SetItemTooltip("Number of threads used to search for text matches");

		ImGui::DragInt("File Size", (int*)&max_file_size, 50.0f, 1, 10000, "%dMB");
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

		const char* name = "Text-Search";
		const char* link = AUTHOR " " CONTACT;

		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 64) / 2.0f);
		ImGui::Image((void*)(intptr_t)img_logo.id, {64, 64});
		ImGui::Dummy({0, 20});

		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(name).x)/2.0f);
		ImGui::Text("%s", name);
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(link).x)/2.0f);
		ImGui::Text("%s", AUTHOR);
		ImGui::SameLine();
		ImGui::TextColored({0,0,0,0.4f}, "%s", CONTACT);
		ImGui::Dummy({0, 20});
		
		if (ImGui::CollapsingHeader("License")) {
			char* license = (char*)_binary_LICENSE_start;
			int64_t license_length = _binary_LICENSE_end - _binary_LICENSE_start;
			ImGui::Text("%.*s", (int)license_length, license);
		}

		ImGui::SeparatorText("Dependencies");
		{
			if (ImGui::TreeNode("https://github.com/ocornut/imgui")) {
				char* license = (char*)_binary_imgui_LICENSE_start;
				int64_t license_length = _binary_imgui_LICENSE_end - _binary_imgui_LICENSE_start;
				ImGui::Text("%.*s", (int)license_length, license);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("https://github.com/dfranx/ImFileDialog")) {
				char* license = (char*)_binary_imfiledialog_LICENSE_start;
				int64_t license_length = _binary_imfiledialog_LICENSE_end - _binary_imfiledialog_LICENSE_start;
				ImGui::Text("%.*s", (int)license_length, license);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("https://github.com/glfw/glfw")) {
				char* license = (char*)_binary_glfw_LICENSE_start;
				int64_t license_length = _binary_glfw_LICENSE_end - _binary_glfw_LICENSE_start;
				ImGui::Text("%.*s", (int)license_length, license);
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

static int _ts_create_menu(int window_w, int window_h) {
	int menu_bar_h = 0;
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::Spectrum::Color(0xDDDDDD));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open")) {
				ifd::FileDialog::Instance().Save("FileOpenDialog", "Open file", "File (*.csv){.csv}");
			}
			if (ImGui::MenuItem("Save")) {
				if (strlen(save_path) == 0)
					ifd::FileDialog::Instance().Save("FileSaveAsDialog", "Save results to file", "File (*.csv;*.json;*.xml){.csv,.json,.xml}");
				else 
					last_export_result = ts_export_result(current_search_result, (const utf8_int8_t *)save_path);
			}
			if (ImGui::MenuItem("Save As")) {
				ifd::FileDialog::Instance().Save("FileSaveAsDialog", "Save results to file", "File (*.csv;*.json;*.xml){.csv,.json,.xml}");
			}
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

	// File exporting.
	if (ifd::FileDialog::Instance().IsDone("FileSaveAsDialog", window_w, window_h)) {
		if (ifd::FileDialog::Instance().HasResult()) {
			std::string res = ifd::FileDialog::Instance().GetResult().u8string();
			last_export_result = ts_export_result(current_search_result, (const utf8_int8_t *)res.c_str());
			utf8ncpy(save_path, (const utf8_int8_t *)res.c_str(), sizeof(save_path));

			// Set titlebar name.
			utf8_int8_t new_name[MAX_INPUT_LENGTH];
			snprintf(new_name, MAX_INPUT_LENGTH, "Text-Search > %s", res.c_str());
			ts_platform_set_window_title(new_name);
		}
		ifd::FileDialog::Instance().Close();
	}

	// File importing.
	if (ifd::FileDialog::Instance().IsDone("FileOpenDialog", window_w, window_h)) {
		if (ifd::FileDialog::Instance().HasResult()) {
			std::string res = ifd::FileDialog::Instance().GetResult().u8string();
			utf8ncpy(save_path, (const utf8_int8_t *)res.c_str(), sizeof(save_path));

			// Set titlebar name.
			utf8_int8_t new_name[MAX_INPUT_LENGTH];
			snprintf(new_name, MAX_INPUT_LENGTH, "Text-Search > %s", res.c_str());
			ts_platform_set_window_title(new_name);

			current_search_result = ts_import_result(save_path);
		}
		ifd::FileDialog::Instance().Close();
	}

	if (last_export_result != EXPORT_NONE) {
		ImGui::OpenPopup("Export Failed");
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	}

	// export error popup
	if (ImGui::BeginPopupModal("Export Failed", (bool*)&last_export_result, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
		ImGui::SetWindowSize({300, 0});

		switch (last_export_result)
		{
			case EXPORT_NO_RESULT: ImGui::Text("No results to export"); break;
			case EXPORT_SEARCH_ACTIVE: ImGui::Text("Can't export while save is active"); break;
			case EXPORT_SAVE_PENDING: ImGui::Text("Export is pending"); break;
		
		default:
			break;
		}

		ImGui::Dummy({0, 20});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Button("Close")) {
			last_export_result = EXPORT_NONE;
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	return menu_bar_h + 15;
}

void ts_init() {
	memset(save_path, 0, sizeof(save_path));
	memset(path_buffer, 0, sizeof(path_buffer));
	memset(filter_buffer, 0, sizeof(filter_buffer));
	memset(query_buffer, 0, sizeof(query_buffer));
}

int _tb_query_input_cb(ImGuiInputTextCallbackData* data) {
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		utf8ncpy(query_buffer, data->Buf, MAX_INPUT_LENGTH);
		ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size, respect_capitalization);
	}

	return 0;
}

void _ts_create_file_match_rows() {
	uint32_t itemcount = current_search_result == 0 ? 0 : current_search_result->files.length;
	for (uint32_t item = 0; item < itemcount; item++)
	{
		ts_found_file *file = *(ts_found_file **)ts_array_at(&current_search_result->files, item);

		char match_info_txt[20];
		snprintf(match_info_txt, 20, "#%u", item+1);
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableHeader(match_info_txt);

		ImGui::TableNextColumn();
		ImGui::TableHeader(file->path);

		ImGui::TableNextColumn();	
		ImGui::TableHeader("");

		ImGui::SameLine();
		ImGui::Selectable("##nolabel", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
	}
}

const utf8_int8_t* _ts_file_error_to_message(ts_file_open_error err) {
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
		default: return "";
	}
}

void _ts_create_file_error_rows() {
	uint32_t itemcount = current_search_result == 0 ? 0 : current_search_result->files.length;
	for (uint32_t item = 0; item < itemcount; item++)
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

		ImGui::SameLine();
		ImGui::Selectable("##nolabel", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
	}
}

void _ts_create_text_match_rows() {
	uint32_t itemcount = current_search_result == 0 ? 0 : current_search_result->matches.length;
	ts_found_file* prev_file = nullptr;
	for (uint32_t item = 0; item < itemcount; item++)
	{
		ts_file_match *file = (ts_file_match *)ts_array_at(&current_search_result->matches, item);

		if (prev_file != file->file) {
			prev_file = file->file;
			char match_info_txt[20];

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			
			ImGui::SetCursorPosX(5);
			ImGui::PushStyleColor(ImGuiCol_Text, {0,0,0,0.1f});
			ImGui::TableHeader(file->file->collapsed ? "▶" : "▼");
			ImGui::PopStyleColor();

			ImGui::TableNextColumn();
			ImGui::TableHeader(file->file->path);

			ImGui::TableNextColumn();	
			snprintf(match_info_txt, 20, "%u match(es)", file->file->match_count);
			ImGui::TableHeader(match_info_txt);

			ImGui::SameLine();
			ImGui::Selectable("##nolabel", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
			if (ImGui::IsItemClicked(ImGuiPopupFlags_MouseButtonLeft)) {
				file->file->collapsed = !file->file->collapsed;
			}
		}

		if (file->file->collapsed) continue;

		char match_nr[20];
		snprintf(match_nr, 20, "#%u", item+1);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%s", match_nr);
					
		ImGui::TableNextColumn();
		
		utf8_int32_t iter_ch = 0;
		utf8_int8_t* iter = file->line_info;
		size_t whitespace_size = 0;
		while ((iter = utf8codepoint(iter, &iter_ch)) && iter_ch)
		{
			if (iter_ch == ' ') {
				ImGui::TextColored({0,0,0,0.2f}, "%s", "→");
				ImGui::SameLine(0.0f, 5.0f);
				whitespace_size++;
			}
			else {
				break;
			}
		}

		ImGui::Text("%.*s", (int)(file->word_match_offset - whitespace_size), file->line_info + whitespace_size);
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::TextColored({255,0,0,255}, "%.*s", (int)file->word_match_length, file->line_info + file->word_match_offset); 
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::TextUnformatted(file->line_info + file->word_match_offset + file->word_match_length);	

		ImGui::SameLine();
		ImGui::Selectable("##nolabel", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

		if (ImGui::IsItemClicked(ImGuiPopupFlags_MouseButtonRight)) {
			ImGui::OpenPopup(match_nr);
		}

		if (ImGui::BeginPopup(match_nr)) {
#if defined(_WIN32)
			if (ImGui::MenuItem("Open as"))
			{
				ts_platform_open_file_as(file->file->path);
			}
			if (ImGui::MenuItem("Open folder")) {
				ts_platform_open_file_in_folder(file->file->path); 
			}
#endif
			if (ImGui::MenuItem("Copy path"))
			{
				ImGui::SetClipboardText(file->file->path);
			}
			if (ImGui::MenuItem("Copy line"))
			{
				ImGui::SetClipboardText(file->line_info);
			}
			ImGui::EndPopup();
		}

		ImGui::TableNextColumn();
		ImGui::Text("line %d", file->line_nr);	
	}
	_ts_create_file_error_rows();
}

int _ts_create_textbox_area(int window_w, int window_h, int textbox_area_height, float pos_y)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

	float offset = 15.0f;
	float separator_w = 10.0f;
	float frame_w = window_w/2.5f - offset - separator_w/2.0f;
	ImGui::SetNextWindowPos({offset, pos_y});
	ImGui::BeginChild("search-boxes", ImVec2((float)frame_w, (float)textbox_area_height), false);
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
			ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size, respect_capitalization);
		}
		ImGui::PopItemWidth();
		ImGui::SetItemTooltip("Text to search within files, supports '*' & '?' wildcards");
		ImGui::PopStyleVar();
	}
	ImGui::EndChild();

	ImGui::SetNextWindowPos({offset + frame_w + separator_w, pos_y});
	ImGui::BeginChild("search-boxes2", ImVec2((float)frame_w, (float)textbox_area_height), false);
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::ImageButton("Folder", (void*)(intptr_t)img_folder.id, ImVec2(18.0f, 18.0f))) {
			ifd::FileDialog::Instance().Open("FolderSelectDialog", "Select a directory", "");
		}
		if (ifd::FileDialog::Instance().IsDone("FolderSelectDialog", window_w, window_h)) {
			if (ifd::FileDialog::Instance().HasResult()) {
				std::string res = ifd::FileDialog::Instance().GetResult().u8string();
				snprintf(path_buffer, MAX_INPUT_LENGTH, "%s", res.c_str());
			}
			ifd::FileDialog::Instance().Close();
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if (ImGui::InputTextWithHint("filter-ti", "Filter", filter_buffer, MAX_INPUT_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size, respect_capitalization);
		}
		ImGui::PopItemWidth();
		ImGui::SetItemTooltip("Files to filter, supports '*' & '?' wildcards");
		ImGui::PopStyleVar();

		if (current_search_result && !current_search_result->search_completed) {
			ImGui::Text("%c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
		}
		else {
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			if (ImGui::ImageButton("Search", (void*)(intptr_t)img_search.id, ImVec2(18.0f, 18.0f))) {
				ts_start_search(path_buffer, filter_buffer, query_buffer, ts_thread_count, max_file_size, respect_capitalization);
			}
			ImGui::PopStyleVar();
		}
		
		ImGui::SameLine();
		ImGui::SetCursorPosX(36);
		ImGui::ToggleButton("Aa", &respect_capitalization);
		ImGui::SetItemTooltip("Match Case");
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();

	return textbox_area_height + 7;
}

void _ts_update_dragdrop() {
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern))	// we use an external source (i.e. not ImGui-created)
	{
		ImGui::SetDragDropPayload("FILES", nullptr, 0);
		ImGui::EndDragDropSource();
	}

	{
		float img_size = 256.0f;
		ImGui::Separator();
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - img_size) / 2.0f);
		ImGui::SetCursorPosY((ImGui::GetWindowHeight() - img_size) / 2.0f);
		ImGui::Image((void*)(intptr_t)img_drop.id, {img_size, img_size}, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 0.2f));
	}
}

void _ts_create_statusbar(int window_w, int window_h, int statusbar_area_height, float pos_y) {
	ImGui::SetNextWindowPos({0, pos_y});

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
	ImGui::BeginChild("search-statusbar", ImVec2((float)window_w, (float)statusbar_area_height), ImGuiChildFlags_None, ImGuiWindowFlags_None);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 7.0f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
	if (current_search_result) {
		if (current_search_result->search_text) ImGui::Text("Found %d matches in %d files", current_search_result->match_count, current_search_result->file_count);
		else ImGui::Text("Found %d files", current_search_result->files.length);
	}
	else ImGui::Text("No search completed");

	ImGui::SameLine();

	if (current_search_result) {
		if (current_search_result->is_saving) {
			ImGui::SetCursorPosX(window_w - 20.0f - ImGui::CalcTextSize("Saving |").x);
			ImGui::Text("Saving %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
		}
		else if (current_search_result->search_completed) {
			ImGui::SetCursorPosX(window_w - 10.0f - ImGui::CalcTextSize("999.999s elapsed").x);
			ImGui::Text("%.3fs elapsed", current_search_result->timestamp/1000.0f);
		}		
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void ts_create_gui(int window_w, int window_h) {
	int window_pad = 50;
	int textbox_area_height = 80;
	int statusbar_area_height = 30;
	float pos_y = 0;
	int result_area_height = window_h - textbox_area_height - statusbar_area_height - window_pad;

	ImGui::SetNextWindowSize({(float)window_w, (float)window_h});
	ImGui::SetNextWindowPos({0, 0});

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));
	ImGui::Begin("text-search", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar|
		ImGuiWindowFlags_MenuBar);
	ImGui::PopStyleVar();

	pos_y += _ts_create_menu(window_w, window_h);
	pos_y += _ts_create_textbox_area(window_w, window_h, textbox_area_height, pos_y);

	if (dragdrop_data.did_drop) {
		current_search_result = ts_import_result(dragdrop_data.path);
		utf8ncpy(save_path, dragdrop_data.path, sizeof(save_path));
		dragdrop_data.did_drop = false;
	}

	if (dragdrop_data.is_dragging_file)
	{
		_ts_update_dragdrop();
	}
	else if (current_search_result)
	{ // Results
		ImGui::SetNextWindowPos({5, pos_y});

		if (ImGui::BeginTable("results-table", 3, ImGuiTableFlags_BordersH|ImGuiTableFlags_ScrollY|ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingFixedFit,
			{(float)window_w-7.0f, (float)result_area_height}))
		{
			float nr_w = 50.0f;
			float line_w = 180.0f;
			float file_w = ImGui::GetWindowWidth() - line_w - nr_w;
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoHeaderLabel, nr_w);
			ImGui::TableSetupColumn("File", 0, file_w);
			ImGui::TableSetupColumn("Match", 0, line_w);			
			ImGui::TableHeadersRow();

			
			if (current_search_result->search_text == nullptr) _ts_create_file_match_rows();
			else _ts_create_text_match_rows();

			if (current_search_result->search_completed && (current_search_result->files.length == 0 || current_search_result->match_count == 0)) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%s", "");
				ImGui::TableNextColumn();

				const char* msg = "No matches found.";
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(msg).x)/2.0f);
				ImGui::TextWrapped("%s", msg);
			}		
		
			ImGui::EndTable();
		}
	}
	else { // Help text
		ImGui::Separator();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
		ImGui::TextWrapped("%s", help_text);
	}
	pos_y += result_area_height;

	_ts_create_statusbar(window_w, window_h, statusbar_area_height, pos_y);

	ImGui::PopStyleVar();
	ImGui::End();
}