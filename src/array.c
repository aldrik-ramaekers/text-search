/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This file is part of Text-search.
*
    *  Text-search is free software: you can redistribute it and/or modify
    *  it under the terms of the GNU General Public License as published by
    *  the Free Software Foundation, either version 3 of the License, or
    *  (at your option) any later version.
	
    *  Text-search is distributed in the hope that it will be useful,
    *  but WITHOUT ANY WARRANTY; without even the implied warranty of
    *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    *  GNU General Public License for more details.
	
    *  You should have received a copy of the GNU General Public License
    *  along with Text-search.  If not, see <https://www.gnu.org/licenses/>.
*/

array array_create(u16 entry_size)
{
	array new_array;
	new_array.length = 0;
	new_array.reserved_length = 0;
	new_array.entry_size = entry_size;
	new_array.data = 0;
	new_array.reserve_jump = 1;
	new_array.mutex = mutex_create();
	
	return new_array;
}

int array_push(array *array, void *data)
{
	assert(array);
	assert(data);
	assert(array->reserve_jump >= 1);
	
	mutex_lock(&array->mutex);
	array->length++;
	
	if (!array->data)
	{
		array->data = mem_alloc(array->entry_size * array->reserve_jump);
		array->reserved_length = array->reserve_jump;
	}
	
	if (array->reserved_length < array->length)
	{
		array->reserved_length += array->reserve_jump;
		array->data = mem_realloc(array->data, (array->reserved_length*array->entry_size));
	}
	
	memcpy(array->data + ((array->length-1) * array->entry_size),
		   data, array->entry_size);
	
	s32 result = array->length -1;
	mutex_unlock(&array->mutex);
	return result;
}

int array_push_size(array *array, void *data, s32 data_size)
{
	assert(array);
	assert(data);
	assert(array->reserve_jump >= 1);
	
	mutex_lock(&array->mutex);
	array->length++;
	
	if (!array->data)
	{
		array->data = mem_alloc(array->entry_size * array->reserve_jump);
		array->reserved_length = array->reserve_jump;
	}
	
	if (array->reserved_length < array->length)
	{
		array->reserved_length += array->reserve_jump;
		array->data = mem_realloc(array->data, (array->reserved_length*array->entry_size));
	}
	
	memcpy(array->data + ((array->length-1) * array->entry_size),
		   data, data_size);
	
	// fill remaining space with 0
	if (array->entry_size > data_size)
	{
		s32 remaining = array->entry_size - data_size;
		memset(array->data + ((array->length-1) * array->entry_size) + data_size,
			   0, remaining);
	}
	
	s32 result = array->length -1;
	mutex_unlock(&array->mutex);
	return result;
}

void array_reserve(array *array, u32 reserve_count)
{
	assert(array);
	
	mutex_lock(&array->mutex);
	u32 reserved_count = array->reserved_length - array->length;
	reserve_count -= reserved_count;
	
	if (reserve_count > 0)
	{
		array->reserved_length += reserve_count;
		array->data = mem_realloc(array->data, (array->reserved_length*array->entry_size));
	}
	mutex_unlock(&array->mutex);
}

void array_remove_at(array *array, u32 at)
{
	assert(array);
	assert(at >= 0);
	assert(at < array->length);
	
	mutex_lock(&array->mutex);
	if (array->length > 1)
	{
		int offset = at * array->entry_size;
		int size = (array->length - at - 1) * array->entry_size;
		memcpy(array->data + offset,
			   array->data + offset + array->entry_size,
			   size);
		
		//array->data = realloc(array->data, array->length * array->entry_size);
	}
	
	array->length--;
	mutex_unlock(&array->mutex);
}

void array_remove(array *array, void *ptr)
{
	mutex_lock(&array->mutex);
	int offset = ptr - array->data;
	int at = offset / array->entry_size;
	array_remove_at(array, at);
	mutex_unlock(&array->mutex);
}

void array_remove_by(array *array, void *data)
{
	assert(array);
	
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

void *array_at(array *array, u32 at)
{
	mutex_lock(&array->mutex);
	assert(array);
	assert(at >= 0);
	assert(at < array->length);
	
	void *result =  array->data + (at * array->entry_size);
	mutex_unlock(&array->mutex);
	return result;
}

void array_destroy(array *array)
{
	assert(array);
	mem_free(array->data);
	mutex_destroy(&array->mutex);
}

void array_swap(array *array, u32 swap1, u32 swap2)
{
	assert(array);
	assert(swap2 >= 0);
	assert(swap2 < array->length);
	if (swap1 == swap2) return;
	
	void *swap1_at = array_at(array, swap1);
	void *swap2_at = array_at(array, swap2);
	
	mutex_lock(&array->mutex);
	char swap1_buffer[array->entry_size];
	memcpy(swap1_buffer, swap1_at, array->entry_size);
	memcpy(swap1_at, swap2_at, array->entry_size);
	memcpy(swap2_at, swap1_buffer, array->entry_size);
	mutex_unlock(&array->mutex);
}

array array_copy(array *arr)
{
	array new_array;
	new_array.length = arr->length;
	new_array.reserved_length = arr->reserved_length;
	new_array.entry_size = arr->entry_size;
	new_array.data = mem_alloc(new_array.entry_size*new_array.reserved_length);
	new_array.mutex = mutex_create();
	
	mutex_lock(&arr->mutex);
	memcpy(new_array.data, arr->data, new_array.entry_size*new_array.reserved_length);
	mutex_unlock(&arr->mutex);
	return new_array;
}