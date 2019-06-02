array array_create(u16 entry_size)
{
	array new_array;
	new_array.length = 0;
	new_array.reserved_length = 0;
	new_array.entry_size = entry_size;
	new_array.data = 0;
	new_array.mutex = mutex_create();
	
	return new_array;
}

int array_push(array *array, void *data)
{
	assert(array);
	assert(data);
	
	mutex_lock(&array->mutex);
	array->length++;
	
	if (!array->data)
	{
		array->data = malloc(array->entry_size);
		array->reserved_length = 1;
	}
	
	if (array->reserved_length < array->length)
	{
		array->reserved_length = array->length;
		array->data = realloc(array->data, (array->reserved_length*array->entry_size));
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
	
	mutex_lock(&array->mutex);
	array->length++;
	
	if (!array->data)
	{
		array->data = malloc(array->entry_size);
		array->reserved_length = 1;
	}
	
	if (array->reserved_length < array->length)
	{
		array->reserved_length = array->length;
		array->data = realloc(array->data, (array->reserved_length*array->entry_size));
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
		array->data = realloc(array->data, (array->reserved_length*array->entry_size));
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
	free(array->data);
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