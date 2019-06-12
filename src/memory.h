#ifndef INCLUDE_MEMORY
#define INCLUDE_MEMORY

#ifdef MODE_DEVELOPER
void *mem_alloc_d(size_t size, const char *caller_name, s32 caller_line);
void mem_free_d(void *ptr, const char *caller_name, s32 caller_line);
void *mem_realloc_d(void *ptr, size_t size, const char *caller_name, s32 caller_line);
void memory_print_leaks();

typedef struct t_memory_usage_entry
{
	const char *name;
	s32 line;
	s32 size;
	void *ptr;
} memory_usage_entry;

array global_memory_usage;
mutex global_memory_mutex;

#define mem_alloc(size) mem_alloc_d(size, __FUNCTION__, __LINE__)
#define mem_free(p) mem_free_d(p, __FUNCTION__, __LINE__)
#define mem_realloc(p, size) mem_realloc_d(p, size, __FUNCTION__, __LINE__)
#else
#define mem_alloc(size) malloc(size)
#define mem_free(p) free(p)
#define mem_realloc(p, size) realloc(p, size)
#endif

#endif