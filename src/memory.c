/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.

*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.

*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#if defined(MODE_DEVELOPER)

static u8 initializing = false;
inline void *mem_alloc_d(size_t size, const char *caller_name, s32 caller_line)
{
	if (global_memory_usage.data == 0 && !initializing)
	{
		initializing = true;
		global_memory_usage = array_create(sizeof(memory_usage_entry));
		array_reserve(&global_memory_usage, 50000);
		global_memory_mutex = mutex_create();
	}
	
	memory_usage_entry entry;
	if (global_memory_usage.data)
	{
		char *complete_stacktrace = malloc(100);
		complete_stacktrace[0] = 0;
		strcpy(complete_stacktrace, caller_name);
		
		entry.name = complete_stacktrace;
		entry.line = caller_line;
		entry.size = size;
	}
	
	void *result = malloc(size);
	
	if (global_memory_usage.data)
	{
		entry.ptr = result;
		
		array_push(&global_memory_usage, &entry);
	}
	
	return result;
}

inline void* mem_realloc_d(void *ptr, size_t size, const char *caller_name, s32 caller_line)
{
    if (global_memory_usage.data == 0 && !initializing)
	{
		initializing = true;
		global_memory_usage = array_create(sizeof(memory_usage_entry));
		array_reserve(&global_memory_usage, 50000);
		global_memory_mutex = mutex_create();
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
			free(entry->name);
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
		free(entry->name);
	}
	mutex_unlock(&global_memory_mutex);
	array_destroy(&global_memory_usage);
	mutex_destroy(&global_memory_mutex);
}

#endif
