#if defined(MODE_DEVELOPER) && defined(OS_LINUX)
#include <execinfo.h>

static void add_stacktrace_line(char *buffer, char *trace)
{
	s32 offset = 0;
	s32 len = 0;
	char *orig = trace;
	
	s32 index = 0;
	while(*trace)
	{
		if (*trace == '(') offset = index;
		if (*trace == '+')
		{
			len = index-offset;
			break;
		}
		
		++index;
		++trace;
	}
	
	strcat(buffer, "     - ");
	strncat(buffer, orig+offset+1, len-1);
	strcat(buffer, "\n");
}

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
	
	char *complete_stacktrace = malloc(3000);
	complete_stacktrace[0] = 0;
	
	void* callstack[128];
	int i, frames = backtrace(callstack, 128);
	char** strs = backtrace_symbols(callstack, frames);
	for (i = 0; i < frames; ++i) {
		add_stacktrace_line(complete_stacktrace, strs[i]);
		//strcat(complete_stacktrace, strs[i]);
		//strcat(complete_stacktrace, "\n");
	}
	free(strs);
	
	strcat(complete_stacktrace, "---- ");
	strcat(complete_stacktrace, caller_name);
	
	memory_usage_entry entry;
	entry.name = complete_stacktrace;
	entry.line = caller_line;
	entry.size = size;
	
	void *result = malloc(size);
	entry.ptr = result;
	
	array_push(&global_memory_usage, &entry);
	
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
