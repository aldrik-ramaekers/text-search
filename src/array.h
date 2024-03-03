#ifndef INCLUDE_ARRAY
#define INCLUDE_ARRAY

#define ASSERT(e_) {if(!(e_)){*(int*)0=0;}}

#include "mutex.h"

typedef struct t_array
{
	int length;
	int reserved_length;
	int entry_size;
	int reserve_jump;
	void *data;
	mutex mutex;
} array;

array array_create(int entry_size);
int array_push(array *array, void *data);
int array_push_size(array *array, void *data, int data_size);
void array_remove_at(array *array, int at);
void array_remove(array *array, void *ptr);
void array_remove_by(array *array, void *data);
void *array_at(array *array, int at);
void array_destroy(array *array);
void array_reserve(array *array, int reserve_count);
array array_copy(array *array);

#endif