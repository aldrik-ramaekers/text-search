#include "config.h"
#include "../imgui/imgui_internal.h"
#include "search.h"

#include <stdio.h>
#include <string.h>
#include <cstring>

utf8_int8_t path_buffer[MAX_INPUT_LENGTH];
utf8_int8_t filter_buffer[MAX_INPUT_LENGTH];
utf8_int8_t query_buffer[MAX_INPUT_LENGTH];
int ts_thread_count = 4;
int max_file_size = 100; // in MBs
bool respect_capitalization = false;

static void _ts_config_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    uint8_t path[MAX_INPUT_LENGTH];
	uint8_t filter[MAX_INPUT_LENGTH];
	uint8_t query[MAX_INPUT_LENGTH];

	int threads = 1, maxSize = 100, matchCase = 0;

#if defined(_WIN32)
    if (sscanf_s(line, "Path=%s", (char*)&path, MAX_INPUT_LENGTH) == 1) { strncpy_s(path_buffer, MAX_INPUT_LENGTH, (char*)path, MAX_INPUT_LENGTH); }
    else if (sscanf_s(line, "Filter=%s", (char*)&filter, MAX_INPUT_LENGTH) == 1) { strncpy_s(filter_buffer, MAX_INPUT_LENGTH, (char*)filter, MAX_INPUT_LENGTH); }
    else if (sscanf_s(line, "Query=%s", (char*)&query, MAX_INPUT_LENGTH) == 1) { strncpy_s(query_buffer, MAX_INPUT_LENGTH, (char*)query, MAX_INPUT_LENGTH); }
	else if (sscanf_s(line, "Threads=%d", &threads) == 1) { ts_thread_count = threads; }
	else if (sscanf_s(line, "MaxSize=%d", &maxSize) == 1) { max_file_size = maxSize; }
	else if (sscanf_s(line, "MatchCase=%d", &matchCase) == 1) { respect_capitalization = matchCase; }
#elif defined(__linux__) || defined(__APPLE__)
	if (sscanf(line, "Path=%s", (char*)&path) == 1) { strncpy(path_buffer, (char*)path, MAX_INPUT_LENGTH); }
    else if (sscanf(line, "Filter=%s", (char*)&filter) == 1) { strncpy(filter_buffer, (char*)filter, MAX_INPUT_LENGTH); }
    else if (sscanf(line, "Query=%s", (char*)&query) == 1) { strncpy(query_buffer, (char*)query, MAX_INPUT_LENGTH); }
	else if (sscanf(line, "Threads=%d", &threads) == 1) { ts_thread_count = threads; }
	else if (sscanf(line, "MaxSize=%d", &maxSize) == 1) { max_file_size = maxSize; }
	else if (sscanf(line, "MatchCase=%d", &matchCase) == 1) { respect_capitalization = matchCase; }
#endif
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
	buf->appendf("MatchCase=%d\n", respect_capitalization);
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