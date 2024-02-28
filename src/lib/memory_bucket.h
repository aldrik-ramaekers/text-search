/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#define kilobytes(num) num*1000
#define megabytes(num) kilobytes(num*1000)

typedef struct t_memory_bucket_entry
{
	char *data;
	s32 length;
	s32 cursor;
} memory_bucket_entry;

typedef struct t_memory_bucket
{
	mutex bucket_mutex;
	array buckets;
} memory_bucket;

memory_bucket memory_bucket_init(s32 bucket_size);
void* memory_bucket_reserve(memory_bucket *bucket, s32 reserve_length);
void memory_bucket_reset(memory_bucket *bucket);
void memory_bucket_destroy(memory_bucket *bucket);