#ifndef INCLUDE_MEMORY_BUCKET
#define INCLUDE_MEMORY_BUCKET

#define kilobytes(num) num*1000
#define megabytes(num) kilobytes(num*1000)

#include "mutex.h"
#include "array.h"

typedef struct t_memory_bucket_entry
{
	char *data;
	int length;
	int cursor;
} memory_bucket_entry;

typedef struct t_memory_bucket
{
	mutex bucket_mutex;
	array buckets;
} memory_bucket;

memory_bucket memory_bucket_init(int bucket_size);
void* memory_bucket_reserve(memory_bucket *bucket, int reserve_length);
void memory_bucket_reset(memory_bucket *bucket);
void memory_bucket_destroy(memory_bucket *bucket);

#endif