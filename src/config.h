#pragma once

#if defined(_WIN32)
#define MAX_INPUT_LENGTH 4096
#elif defined(__linux__)
#include <stdio.h>
#define MAX_INPUT_LENGTH 4096
#elif defined(__APPLE__)
#include <stdio.h>
#define MAX_INPUT_LENGTH 1024
#endif

#define exit_oom() { perror("Out of memory."); exit(-1); }

#define AUTHOR "created by Aldrik Ramaekers"
#define CONTACT "<aldrik.ramaekers@gmail.com>"

#define MAX_ERROR_MESSAGE_LENGTH (MAX_INPUT_LENGTH)
#define FILE_RESERVE_COUNT 1000

#include "../imgui/imgui.h"
#include "../utf8.h"

// Current session only.
extern utf8_int8_t save_path[MAX_INPUT_LENGTH];

// Stored in config.
extern utf8_int8_t path_buffer[MAX_INPUT_LENGTH];
extern utf8_int8_t filter_buffer[MAX_INPUT_LENGTH];
extern utf8_int8_t query_buffer[MAX_INPUT_LENGTH];
extern uint16_t ts_thread_count;
extern uint32_t max_file_size; // in MBs
extern bool respect_capitalization;

void ts_load_config();