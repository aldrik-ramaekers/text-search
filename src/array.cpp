#include "array.h"

array array_create(int entry_size)
{
	array new_array;
	new_array.length = 0;
	new_array.reserved_length = 0;
	new_array.entry_size = entry_size;
	new_array.data = 0;
	new_array.reserve_jump = 1;
	new_array.mutex = mutex_create_recursive();
	
	return new_array;
}

int array_push(array *array, void *data)
{
	ASSERT(array);
	ASSERT(data);
	ASSERT(array->reserve_jump >= 1);
	
	mutex_lock(&array->mutex);
	array->length++;
	
	if (!array->data)
	{
		array->data = malloc(array->entry_size * array->reserve_jump);
		array->reserved_length = array->reserve_jump;
	}
	
	if (array->reserved_length < array->length)
	{
		array->reserved_length += array->reserve_jump;
		array->data = realloc(array->data, (array->reserved_length*array->entry_size));
	}
	
	memcpy((char*)array->data + ((array->length-1) * array->entry_size),
		   data, array->entry_size);
	
	int result = array->length -1;
	mutex_unlock(&array->mutex);
	return result;
}

int array_push_size(array *array, void *data, int data_size)
{
	ASSERT(array);
	ASSERT(data);
	ASSERT(array->reserve_jump >= 1);
	
	mutex_lock(&array->mutex);
	array->length++;
	
	if (!array->data)
	{
		array->data = malloc(array->entry_size * array->reserve_jump);
		array->reserved_length = array->reserve_jump;
	}
	
	if (array->reserved_length < array->length)
	{
		array->reserved_length += array->reserve_jump;
		array->data = realloc(array->data, (array->reserved_length*array->entry_size));
	}
	
	memcpy((char*)array->data + ((array->length-1) * array->entry_size),
		   data, data_size);
	
	// fill remaining space with 0
	if (array->entry_size > data_size)
	{
		int remaining = array->entry_size - data_size;
		memset((char*)array->data + ((array->length-1) * array->entry_size) + data_size,
			   0, remaining);
	}
	
	int result = array->length -1;
	mutex_unlock(&array->mutex);
	return result;
}

void array_reserve(array *array, int reserve_count)
{
	ASSERT(array);
	
	mutex_lock(&array->mutex);
	int reserved_count = array->reserved_length - array->length;
	reserve_count -= reserved_count;
	
	if (reserve_count > 0)
	{
		array->reserved_length += reserve_count;
		
		if (array->data)
		{
			array->data = realloc(array->data, (array->reserved_length*array->entry_size));
		}
		else
		{
			array->data = malloc(array->reserved_length*array->entry_size);
		}
	}
	mutex_unlock(&array->mutex);
}

void array_remove_at(array *array, int at)
{
	ASSERT(array);
	ASSERT(at >= 0);
	ASSERT(at < array->length);
	
	mutex_lock(&array->mutex);
	if (array->length > 1)
	{
		int offset = at * array->entry_size;
		int size = (array->length - at - 1) * array->entry_size;
		memcpy((char*)array->data + offset,
			   (char*)array->data + offset + array->entry_size,
			   size);
		
		//array->data = realloc(array->data, array->length * array->entry_size);
	}
	
	array->length--;
	mutex_unlock(&array->mutex);
}

void array_remove(array *array, void *ptr)
{
	mutex_lock(&array->mutex);
	int offset = (char*)ptr - (char*)array->data;
	int at = offset / array->entry_size;
	array_remove_at(array, at);
	mutex_unlock(&array->mutex);
}

void array_remove_by(array *array, void *data)
{
	ASSERT(array);
	
	mutex_lock(&array->mutex);
	for (int i = 0; i < array->length; i++)
	{
		void *d = array_at(array, i);
		if (memcmp(d, data, array->entry_size) == 0)
		{
			array_remove_at(array, i);
			return;
		}
	}
	mutex_unlock(&array->mutex);
}

void *array_at(array *array, int at)
{
	mutex_lock(&array->mutex);
	ASSERT(array);
	ASSERT(at >= 0);
	ASSERT(at < array->length);
	
	void *result =  (char*)array->data + (at * array->entry_size);
	mutex_unlock(&array->mutex);
	return result;
}

void array_destroy(array *array)
{
	ASSERT(array);
	free(array->data);
	mutex_destroy(&array->mutex);
}

array array_copy(array *arr)
{
	array new_array;
	new_array.length = arr->length;
	new_array.reserved_length = arr->reserved_length;
	new_array.entry_size = arr->entry_size;
	new_array.data = malloc(new_array.entry_size*new_array.reserved_length);
	new_array.mutex = mutex_create();
	
	mutex_lock(&arr->mutex);
	memcpy(new_array.data, arr->data, new_array.entry_size*new_array.reserved_length);
	mutex_unlock(&arr->mutex);
	return new_array;
}