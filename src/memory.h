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

#ifndef INCLUDE_MEMORY
#define INCLUDE_MEMORY

#if defined(MODE_DEVELOPER) && defined(OS_LINUX)

void *mem_alloc_d(size_t size, const char *caller_name, s32 caller_line);
void mem_free_d(void *ptr, const char *caller_name, s32 caller_line);
void *mem_realloc_d(void *ptr, size_t size, const char *caller_name, s32 caller_line);
void memory_print_leaks();

typedef struct t_memory_usage_entry
{
	char *name;
	s32 line;
	s32 size;
	void *ptr;
} memory_usage_entry;

array global_memory_usage;
mutex global_memory_mutex;

#define mem_alloc(size) mem_alloc_d(size, __FUNCTION__, __LINE__)
#define mem_free(p) mem_free_d(p, __FUNCTION__, __LINE__)
#define mem_realloc(p, size) mem_realloc_d(p, size, __FUNCTION__, __LINE__)

#define STBI_MALLOC(sz) mem_alloc(sz)
#define STBI_REALLOC(p, newsz) mem_realloc(p, newsz)
#define STBI_FREE(p) mem_free(p)

#else
#define mem_alloc(size) malloc(size)
#define mem_free(p) free(p)
#define mem_realloc(p, size) realloc(p, size)
#endif

#endif