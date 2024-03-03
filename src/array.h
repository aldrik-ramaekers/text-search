#ifndef INCLUDE_ts_array
#define INCLUDE_ts_array

#define ASSERT(e_) {if(!(e_)){*(int*)0=0;}}

#include "mutex.h"

typedef struct t_ts_array
{
	int length;
	int reserved_length;
	int entry_size;
	int reserve_jump;
	void *data;
	ts_mutex mutex;
} ts_array;

ts_array 	ts_array_create(int entry_size);
int 		ts_array_push(ts_array *ts_array, void *data);
int 		ts_array_push_size(ts_array *ts_array, void *data, int data_size);
void 		ts_array_remove_at(ts_array *ts_array, int at);
void 		ts_array_remove(ts_array *ts_array, void *ptr);
void 		ts_array_remove_by(ts_array *ts_array, void *data);
void*		ts_array_at(ts_array *ts_array, int at);
void 		ts_array_destroy(ts_array *ts_array);
void 		ts_array_reserve(ts_array *ts_array, int reserve_count);
ts_array 	ts_array_copy(ts_array *ts_array);

#endif