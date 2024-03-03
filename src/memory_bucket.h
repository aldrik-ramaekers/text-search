#ifndef INCLUDE_MEMORY_BUCKET
#define INCLUDE_MEMORY_BUCKET

#define kilobytes(num) num*1000
#define megabytes(num) kilobytes(num*1000)

#include "mutex.h"
#include "array.h"

typedef struct t_ts_memory_bucket_entry
{
	char *data;
	int length;
	int cursor;
} ts_memory_bucket_entry;

typedef struct t_ts_memory_bucket
{
	ts_mutex bucket_mutex;
	ts_array buckets;
} ts_memory_bucket;

ts_memory_bucket 	ts_memory_bucket_init(int bucket_size);
void* 				ts_memory_bucket_reserve(ts_memory_bucket *bucket, int reserve_length);
void 				ts_memory_bucket_reset(ts_memory_bucket *bucket);
void 				ts_memory_bucket_destroy(ts_memory_bucket *bucket);

#endif