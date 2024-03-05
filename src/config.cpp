#include "config.h"
#include "../imgui/imgui_internal.h"
#include "search.h"

#include <stdio.h>

utf8_int8_t path_buffer[MAX_INPUT_LENGTH];
utf8_int8_t filter_buffer[MAX_INPUT_LENGTH];
utf8_int8_t query_buffer[MAX_INPUT_LENGTH];
int ts_thread_count = 4;
int max_file_size = 100; // in MBs

static void _ts_config_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiWindowSettings* settings = (ImGuiWindowSettings*)entry;
    uint8_t path[MAX_INPUT_LENGTH];
	uint8_t filter[MAX_INPUT_LENGTH];
	uint8_t query[MAX_INPUT_LENGTH];

	int threads, maxSize;

    if (sscanf(line, "Path=%s", &path) == 1) { strcpy_s(path_buffer, MAX_INPUT_LENGTH, (char*)path); }
    else if (sscanf(line, "Filter=%s", &filter) == 1) { strcpy_s(filter_buffer, MAX_INPUT_LENGTH, (char*)filter); }
    else if (sscanf(line, "Query=%s", &query) == 1) { strcpy_s(query_buffer, MAX_INPUT_LENGTH, (char*)query); }
	else if (sscanf(line, "Threads=%d", &threads) == 1) { ts_thread_count = threads; }
	else if (sscanf(line, "MaxSize=%d", &maxSize) == 1) { max_file_size = maxSize; }
}

static void _ts_config_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    // Write to text buffer
    buf->reserve(MAX_INPUT_LENGTH*4); // ballpark reserve
    buf->appendf("[%s][%s]\n", handler->TypeName, "Search");
	buf->appendf("Path=%s\n", path_buffer);
	buf->appendf("Filter=%s\n", filter_buffer);
	buf->appendf("Query=%s\n", query_buffer);
	buf->appendf("Threads=%d\n", ts_thread_count);
	buf->appendf("MaxSize=%d\n", max_file_size);
	buf->append("\n");
}

static void* _ts_config_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name) {
	return (void*)0xFFFFFF;
}

static void _ts_config_ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
	
}

void ts_load_config() {
	ImGuiSettingsHandler ini_handler;
	ini_handler.TypeName = "Query";
	ini_handler.TypeHash = ImHashStr("Query");
	ini_handler.ClearAllFn = nullptr;
	ini_handler.ReadOpenFn = _ts_config_ReadOpen;
	ini_handler.ReadLineFn = _ts_config_ReadLine;
	ini_handler.ApplyAllFn = _ts_config_ApplyAll;
	ini_handler.WriteAllFn = _ts_config_WriteAll;
	ImGui::AddSettingsHandler(&ini_handler);
}