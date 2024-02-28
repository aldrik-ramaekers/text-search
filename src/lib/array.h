/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_ARRAY
#define INCLUDE_ARRAY

#define ASSERT(e_) {if(!(e_)){*(int*)0=0;}}

typedef struct t_array
{
	u32 length;
	u32 reserved_length;
	u64 entry_size;
	u32 reserve_jump;
	void *data;
	mutex mutex;
} array;

array array_create(u64 entry_size);
int array_push(array *array, void *data);
int array_push_size(array *array, void *data, s32 data_size);
void array_remove_at(array *array, u32 at);
void array_remove(array *array, void *ptr);
void array_remove_by(array *array, void *data);
void *array_at(array *array, u32 at);
void array_destroy(array *array);
void array_swap(array *array, u32 swap1, u32 swap2);
void array_reserve(array *array, u32 reserve_count);
array array_copy(array *array);

#endif