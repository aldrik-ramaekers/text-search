#pragma once

#define kilobytes(num) num*1000
#define megabytes(num) kilobytes(num*1000)

#include "mutex.h"
#include "array.h"

typedef struct t_ts_memory_bucket_entry
{
	char *data;
	uint32_t length;
	uint32_t cursor;
} ts_memory_bucket_entry;

typedef struct t_ts_memory_bucket
{
	ts_mutex bucket_mutex;
	ts_array buckets;
} ts_memory_bucket;

ts_memory_bucket 	ts_memory_bucket_init(uint32_t bucket_size);
void* 				ts_memory_bucket_reserve(ts_memory_bucket *bucket, uint32_t reserve_length);
void 				ts_memory_bucket_reset(ts_memory_bucket *bucket);
void 				ts_memory_bucket_destroy(ts_memory_bucket *bucket);
