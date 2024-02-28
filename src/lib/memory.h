/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_MEMORY
#define INCLUDE_MEMORY

#ifdef MODE_DEBUGMEM
#include <stdio.h>
#include <dbghelp.h>

#define MEM_ENTRY_BUFFER_SIZE 50000

typedef struct t_mem_entry
{
	bool valid;
	void *p;
	s32 size;
	char *stacktrace;
} __mem_entry;

static __mem_entry *mem_entries;

static s32 __total_allocated = 0;
static s32 __total_reallocated = 0;

static void* __custom_alloc(s32 size)
{
	if (mem_entries == 0) {
		mem_entries = malloc(sizeof(__mem_entry)*MEM_ENTRY_BUFFER_SIZE);
		memset(mem_entries, 0, MEM_ENTRY_BUFFER_SIZE);
	}
	
	void* newp = malloc(size);
	
	bool found = false;
	for (s32 i = 0; i < MEM_ENTRY_BUFFER_SIZE; i++)
	{
		if (!mem_entries[i].valid)
		{
#ifdef OS_WIN
			HANDLE process = GetCurrentProcess();
			HANDLE thread = GetCurrentThread();
			
			CONTEXT context;
			memset(&context, 0, sizeof(CONTEXT));
			context.ContextFlags = CONTEXT_FULL;
			RtlCaptureContext(&context);
			
			SymInitialize(process, NULL, TRUE);
			
			DWORD image;
			STACKFRAME64 stackframe;
			ZeroMemory(&stackframe, sizeof(STACKFRAME64));
			
#ifdef _M_IX86
			image = IMAGE_FILE_MACHINE_I386;
			stackframe.AddrPC.Offset = context.Eip;
			stackframe.AddrPC.Mode = AddrModeFlat;
			stackframe.AddrFrame.Offset = context.Ebp;
			stackframe.AddrFrame.Mode = AddrModeFlat;
			stackframe.AddrStack.Offset = context.Esp;
			stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
			image = IMAGE_FILE_MACHINE_AMD64;
			stackframe.AddrPC.Offset = context.Rip;
			stackframe.AddrPC.Mode = AddrModeFlat;
			stackframe.AddrFrame.Offset = context.Rsp;
			stackframe.AddrFrame.Mode = AddrModeFlat;
			stackframe.AddrStack.Offset = context.Rsp;
			stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
			image = IMAGE_FILE_MACHINE_IA64;
			stackframe.AddrPC.Offset = context.StIIP;
			stackframe.AddrPC.Mode = AddrModeFlat;
			stackframe.AddrFrame.Offset = context.IntSp;
			stackframe.AddrFrame.Mode = AddrModeFlat;
			stackframe.AddrBStore.Offset = context.RsBSP;
			stackframe.AddrBStore.Mode = AddrModeFlat;
			stackframe.AddrStack.Offset = context.IntSp;
			stackframe.AddrStack.Mode = AddrModeFlat;
#endif
			
			mem_entries[i].stacktrace = malloc(4000);
			mem_entries[i].stacktrace[0] = 0;
			
			for (size_t x = 0; i < 25; x++) {
				
				BOOL result = StackWalk64(
					image, process, thread,
					&stackframe, &context, NULL, 
					SymFunctionTableAccess64, SymGetModuleBase64, NULL);
				
				if (!result) { break; }
				
				char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
				PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
				symbol->MaxNameLen = MAX_SYM_NAME;
				
				DWORD64 displacement = 0;
				if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol)) {
					char tmp[100];
					snprintf(tmp, 100, "[%I64i] %s\n", x, symbol->Name);
					strncat(mem_entries[i].stacktrace, tmp, 4000-1);
				} else {
					char tmp[100];
					snprintf(tmp, 100, "[%I64i] ???\n", x);
					strncat(mem_entries[i].stacktrace, tmp, 4000-1);
				}
				
			}
			
			SymCleanup(process);
			
#endif
			
			mem_entries[i].valid = true;
			mem_entries[i].p = newp;
			mem_entries[i].size = size;
			found = true;
			break;
		}
	}
	
	if (!found) assert(0 && "memory entry buffer too small");
	
	__total_allocated+=size;
	return newp;
}

static void* __custom_realloc(void *p, s32 size)
{
	for (s32 i = 0; i < MEM_ENTRY_BUFFER_SIZE; i++)
	{
		if (mem_entries[i].valid && mem_entries[i].p == p)
		{
			__total_allocated-=mem_entries[i].size;
			__total_allocated+=size;
			mem_entries[i].size = size;
			break;
		}
	}
	
	__total_reallocated+=size;
	return realloc(p, size);
}

static void __custom_free(void *p)
{
	for (s32 i = 0; i < MEM_ENTRY_BUFFER_SIZE; i++)
	{
		if (mem_entries[i].valid && mem_entries[i].p == p)
		{
			__total_allocated-=mem_entries[i].size;
			mem_entries[i].valid = false;
			break;
		}
	}
	
	free(p);
}

static void __custom_print_leaks()
{
	printf("\n\n********LEAK LIST********\n");
	for (s32 i = 0; i < MEM_ENTRY_BUFFER_SIZE; i++)
	{
		if (mem_entries[i].valid)
		{
			printf("%p: %d\n", mem_entries[i].p, mem_entries[i].size);
			printf("%s\n", mem_entries[i].stacktrace);
		}
	}
	getch();
}

#define mem_alloc(size) __custom_alloc(size)
#define mem_free(p) __custom_free(p)
#define mem_realloc(p, size) __custom_realloc(p, size);
#define memory_print_leaks() __custom_print_leaks()

#else

#define mem_alloc(size) malloc(size)
#define mem_free(p) free(p)
#define mem_realloc(p, size) realloc(p, size)
#define memory_print_leaks() {}

#endif

#define STBI_MALLOC(sz) mem_alloc(sz)
#define STBI_REALLOC(p, newsz) mem_realloc(p, newsz)
#define STBI_FREE(p) mem_free(p)

#endif