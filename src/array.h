#ifndef INCLUDE_ts_array
#define INCLUDE_ts_array

#include "mutex.h"
#include <cassert>
#include <cstdint>

typedef struct t_ts_array
{
	uint32_t length;
	uint32_t reserved_length;
	uint32_t entry_size;
	uint32_t reserve_jump;
	void *data;
	ts_mutex mutex;
} ts_array;

ts_array 	ts_array_create(uint32_t entry_size);
uint32_t 	ts_array_push(ts_array *ts_array, void *data);
uint32_t 	ts_array_push_size(ts_array *ts_array, void *data, uint32_t data_size);
void 		ts_array_remove_at(ts_array *ts_array, uint32_t at);
void 		ts_array_remove(ts_array *ts_array, void *ptr);
void 		ts_array_remove_by(ts_array *ts_array, void *data);
void*		ts_array_at(ts_array *ts_array, uint32_t at);
void 		ts_array_destroy(ts_array *ts_array);
void 		ts_array_reserve(ts_array *ts_array, uint32_t reserve_count);
ts_array 	ts_array_copy(ts_array *ts_array);

#endif