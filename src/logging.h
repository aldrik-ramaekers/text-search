#pragma once

#include "array.h"
#include "platform.h"
#include "../utf8.h"

#define LOG_ENTRY_SIZE 120

typedef struct t_ts_log
{
	ts_array entries;
	uint64_t start_time;
} ts_log;

extern ts_log logger;

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TS_LOG_TRACE(__format, ...) { \
	if (logger.entries.data == NULL) { \
		logger.start_time = ts_platform_get_time(); \
		logger.entries = ts_array_create(LOG_ENTRY_SIZE); \
		ts_array_reserve(&logger.entries, 50); \
		logger.entries.reserve_jump = 50; \
	} \
	utf8_int8_t __tmp[LOG_ENTRY_SIZE]; \
	utf8_int8_t* __buffer = (utf8_int8_t*)malloc(LOG_ENTRY_SIZE); \
	memset(__buffer, 0, LOG_ENTRY_SIZE); \
	snprintf(__tmp, LOG_ENTRY_SIZE, "[%10.3f] %s", ts_platform_get_time(logger.start_time)/1000.0f, __format); \
	snprintf(__buffer, LOG_ENTRY_SIZE, __tmp, __VA_ARGS__); \
	ts_array_push(&logger.entries, __buffer); \
}