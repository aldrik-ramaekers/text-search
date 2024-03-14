#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#pragma warning(disable : 4996)
void *debug_malloc(size_t size, const char* file, int line) {
	void *p = malloc(size);

	if (p == NULL) {
		return NULL;
	}

	char buff[256];

	sprintf(buff, "bin/debug/mem/%p.mem", p);

	FILE *f = fopen(buff, "w");
	if (!f) return p;
	fprintf(f, "File: %s\nLine: %d\nSize: %zu bytes\n", file, line, size);
	fclose(f);

	return p;
}

void *debug_calloc(size_t count, size_t size, const char* file, int line) {
	void *p = calloc(count, size);

	if (p == NULL) {
		return NULL;
	}

	char buff[256];

	sprintf(buff, "bin/debug/mem/%p.mem", p);

	FILE *f = fopen(buff, "w");
	if (!f) return p;

	fprintf(f, "File: %s\nLine: %d\nSize: %zu bytes\n", file, line, 
		count * size);
	fclose(f);

	return p;
}

void *debug_realloc(void *ptr, size_t size, const char* file, int line) {
	void *p = realloc(ptr, size);

	if (p == NULL) {
		return NULL;
	}

	char buff[256];
	//Delete the old pointer record
	sprintf(buff, "bin/debug/mem/%p.mem", ptr);
	if (ptr != NULL && unlink(buff) < 0) {
		printf("Double free: %p File: %s Line: %d\n", ptr, file, line);
	}

	//Create the new pointer record
	sprintf(buff, "bin/debug/mem/%p.mem", p);

	FILE *f = fopen(buff, "w");
	if (!f) return p;

	fprintf(f, "File: %s\nLine: %d\nSize: %zu bytes\n", file, line, size);
	fclose(f);

	return p;
}

void debug_free(void *p, const char* file, int line) {
	char buff[256];

	sprintf(buff, "bin/debug/mem/%p.mem", p);
	if (p != NULL && unlink(buff) < 0) {
		printf("Double free: %p File: %s Line: %d\n", p, file, line);
	}

	free(p);
}
