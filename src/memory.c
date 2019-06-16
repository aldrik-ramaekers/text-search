#ifdef MODE_DEVELOPER

inline void *mem_alloc_d(size_t size, const char *caller_name, s32 caller_line)
{
	if (global_memory_usage.data == 0)
	{
		global_memory_usage.data = (void*)(1);
		global_memory_usage = array_create(sizeof(memory_usage_entry));
		array_reserve(&global_memory_usage, 50000);
	}
	
	memory_usage_entry entry;
	entry.name = caller_name;
	entry.line = caller_line;
	entry.size = size;
	
	void *result = malloc(size);
	entry.ptr = result;
	
	array_push(&global_memory_usage, &entry);
	
	return result;
}

inline void* mem_realloc_d(void *ptr, size_t size, const char *caller_name, s32 caller_line)
{
    if (global_memory_usage.data == 0)
	{
		global_memory_usage.data = (void*)(1);
		global_memory_usage = array_create(sizeof(memory_usage_entry));
		array_reserve(&global_memory_usage, 50000);
	}
	
	void *result = realloc(ptr, size);
	
	mutex_lock(&global_memory_mutex);
	for (s32 i = 0; i < global_memory_usage.length; i++)
	{
		memory_usage_entry *entry = array_at(&global_memory_usage, i);
		if (entry->ptr == ptr)
		{
			entry->ptr = result;
		}
	}
	mutex_unlock(&global_memory_mutex);
	
	return result;
}

inline void mem_free_d(void *ptr, const char *caller_name, s32 caller_line)
{
	mutex_lock(&global_memory_mutex);
	for (s32 i = 0; i < global_memory_usage.length; i++)
	{
		memory_usage_entry *entry = array_at(&global_memory_usage, i);
		if (entry->ptr == ptr)
		{
			array_remove_at(&global_memory_usage, i);
		}
	}
	mutex_unlock(&global_memory_mutex);
	
	free(ptr);
}

void memory_print_leaks()
{
	mutex_lock(&global_memory_mutex);
	for (s32 i = 0; i < global_memory_usage.length; i++)
	{
		memory_usage_entry *entry = array_at(&global_memory_usage, i);
		printf("%s:%d:%d:%p\n", entry->name, entry->line, entry->size, entry->ptr);
	}
	mutex_unlock(&global_memory_mutex);
	array_destroy(&global_memory_usage);
	mutex_destroy(&global_memory_mutex);
}

#endif
