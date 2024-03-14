#include "array.h"

#include <stdlib.h>
#include <cstring>

ts_array ts_array_create(uint32_t entry_size)
{
	ts_array new_ts_array;
	new_ts_array.length = 0;
	new_ts_array.reserved_length = 0;
	new_ts_array.entry_size = entry_size;
	new_ts_array.data = 0;
	new_ts_array.reserve_jump = 1;
	new_ts_array.mutex = ts_mutex_create_recursive();
	
	return new_ts_array;
}

uint32_t ts_array_push(ts_array *ts_array, void *data)
{
	ASSERT(ts_array);
	ASSERT(data);
	ASSERT(ts_array->reserve_jump >= 1);
	
	ts_mutex_lock(&ts_array->mutex);
	ts_array->length++;
	
	if (!ts_array->data)
	{
		ts_array->data = malloc(ts_array->entry_size * ts_array->reserve_jump);
		ts_array->reserved_length = ts_array->reserve_jump;
	}
	
	if (ts_array->reserved_length < ts_array->length)
	{
		ts_array->reserved_length += ts_array->reserve_jump;
		ts_array->data = realloc(ts_array->data, (ts_array->reserved_length*ts_array->entry_size));
	}
	
	memcpy((char*)ts_array->data + ((ts_array->length-1) * ts_array->entry_size),
		   data, ts_array->entry_size);
	
	uint32_t result = ts_array->length -1;
	ts_mutex_unlock(&ts_array->mutex);
	return result;
}

uint32_t ts_array_push_size(ts_array *ts_array, void *data, uint32_t data_size)
{
	ASSERT(ts_array);
	ASSERT(data);
	ASSERT(ts_array->reserve_jump >= 1);
	
	ts_mutex_lock(&ts_array->mutex);
	ts_array->length++;
	
	if (!ts_array->data)
	{
		ts_array->data = malloc(ts_array->entry_size * ts_array->reserve_jump);
		ts_array->reserved_length = ts_array->reserve_jump;
	}
	
	if (ts_array->reserved_length < ts_array->length)
	{
		ts_array->reserved_length += ts_array->reserve_jump;
		ts_array->data = realloc(ts_array->data, (ts_array->reserved_length*ts_array->entry_size));
	}
	
	memcpy((char*)ts_array->data + ((ts_array->length-1) * ts_array->entry_size),
		   data, data_size);
	
	// fill remaining space with 0
	if (ts_array->entry_size > data_size)
	{
		uint32_t remaining = ts_array->entry_size - data_size;
		memset((char*)ts_array->data + ((ts_array->length-1) * ts_array->entry_size) + data_size,
			   0, remaining);
	}
	
	uint32_t result = ts_array->length -1;
	ts_mutex_unlock(&ts_array->mutex);
	return result;
}

void ts_array_reserve(ts_array *ts_array, uint32_t reserve_count)
{
	ASSERT(ts_array);
	
	ts_mutex_lock(&ts_array->mutex);
	uint32_t reserved_count = ts_array->reserved_length - ts_array->length;
	reserve_count -= reserved_count;
	
	if (reserve_count > 0)
	{
		ts_array->reserved_length += reserve_count;
		
		if (ts_array->data)
		{
			ts_array->data = realloc(ts_array->data, (ts_array->reserved_length*ts_array->entry_size));
		}
		else
		{
			ts_array->data = malloc(ts_array->reserved_length*ts_array->entry_size);
		}
	}
	ts_mutex_unlock(&ts_array->mutex);
}

void ts_array_remove_at(ts_array *ts_array, uint32_t at)
{
	ASSERT(ts_array);
	ASSERT(at >= 0);
	ASSERT(at < ts_array->length);
	
	ts_mutex_lock(&ts_array->mutex);
	if (ts_array->length > 1)
	{
		uint32_t offset = at * ts_array->entry_size;
		uint32_t size = (ts_array->length - at - 1) * ts_array->entry_size;
		memcpy((char*)ts_array->data + offset,
			   (char*)ts_array->data + offset + ts_array->entry_size,
			   size);
		
		//ts_array->data = realloc(ts_array->data, ts_array->length * ts_array->entry_size);
	}
	
	ts_array->length--;
	ts_mutex_unlock(&ts_array->mutex);
}

void ts_array_remove(ts_array *ts_array, void *ptr)
{
	ts_mutex_lock(&ts_array->mutex);
	size_t offset = (char*)ptr - (char*)ts_array->data;
	size_t at = offset / ts_array->entry_size;
	ts_array_remove_at(ts_array, (uint32_t)at);
	ts_mutex_unlock(&ts_array->mutex);
}

void ts_array_remove_by(ts_array *ts_array, void *data)
{
	ASSERT(ts_array);
	
	ts_mutex_lock(&ts_array->mutex);
	for (uint32_t i = 0; i < ts_array->length; i++)
	{
		void *d = ts_array_at(ts_array, i);
		if (memcmp(d, data, ts_array->entry_size) == 0)
		{
			ts_array_remove_at(ts_array, i);
			return;
		}
	}
	ts_mutex_unlock(&ts_array->mutex);
}

void *ts_array_at(ts_array *ts_array, uint32_t at)
{
	ts_mutex_lock(&ts_array->mutex);
	ASSERT(ts_array);
	ASSERT(at >= 0);
	ASSERT(at < ts_array->length);
	
	void *result =  (char*)ts_array->data + (at * ts_array->entry_size);
	ts_mutex_unlock(&ts_array->mutex);
	return result;
}

void ts_array_destroy(ts_array *ts_array)
{
	ASSERT(ts_array);
	free(ts_array->data);
	ts_mutex_destroy(&ts_array->mutex);
}

ts_array ts_array_copy(ts_array *arr)
{
	ts_array new_ts_array;
	new_ts_array.length = arr->length;
	new_ts_array.reserved_length = arr->reserved_length;
	new_ts_array.entry_size = arr->entry_size;
	new_ts_array.data = malloc(new_ts_array.entry_size*new_ts_array.reserved_length);
	new_ts_array.mutex = ts_mutex_create();
	
	ts_mutex_lock(&arr->mutex);
	memcpy(new_ts_array.data, arr->data, new_ts_array.entry_size*new_ts_array.reserved_length);
	ts_mutex_unlock(&arr->mutex);
	return new_ts_array;
}