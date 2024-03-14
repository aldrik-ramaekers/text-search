#include "memory_bucket.h"
#include <stdlib.h>

ts_memory_bucket ts_memory_bucket_init(uint32_t bucket_size)
{
	ts_memory_bucket collection;
	collection.bucket_mutex = ts_mutex_create();
	collection.buckets = ts_array_create(sizeof(ts_memory_bucket_entry));
	
	ts_memory_bucket_entry bucket;
	bucket.data = (char*)malloc(bucket_size);
	bucket.length = bucket_size;
	bucket.cursor = 0;
	ts_array_push(&collection.buckets, &bucket);
	return collection;
}

void* ts_memory_bucket_reserve(ts_memory_bucket *bucket, uint32_t reserve_length)
{
	ts_mutex_lock(&bucket->bucket_mutex);
	ts_memory_bucket_entry *bucket_entry = 0;
	for (uint32_t i = 0; i < bucket->buckets.length; i++)
	{
		bucket_entry = (ts_memory_bucket_entry *)ts_array_at(&bucket->buckets, i);
		
		if (bucket_entry->length - bucket_entry->cursor < reserve_length) continue;
		
		void *space = bucket_entry->data+bucket_entry->cursor;
		bucket_entry->cursor += reserve_length;
		ts_mutex_unlock(&bucket->bucket_mutex);
		
		return space;
	}
	
	// failed to find suitable space, allocate new bucket
	ts_memory_bucket_entry new_bucket;
	new_bucket.data = (char*)malloc(bucket_entry->length);
	new_bucket.length = bucket_entry->length;
	new_bucket.cursor = reserve_length;
	ts_array_push(&bucket->buckets, &new_bucket);
	ts_mutex_unlock(&bucket->bucket_mutex);
	
	return new_bucket.data;
}

void ts_memory_bucket_reset(ts_memory_bucket *bucket)
{
	ts_mutex_lock(&bucket->bucket_mutex);
	for (uint32_t i = 0; i < bucket->buckets.length; i++)
	{
		ts_memory_bucket_entry *bucket_entry = (ts_memory_bucket_entry *)ts_array_at(&bucket->buckets, i);
		bucket_entry->cursor = 0;
	}
	ts_mutex_unlock(&bucket->bucket_mutex);
}

void ts_memory_bucket_destroy(ts_memory_bucket *bucket)
{
	ts_mutex_lock(&bucket->bucket_mutex);
	for (uint32_t i = 0; i < bucket->buckets.length; i++)
	{
		ts_memory_bucket_entry *bucket_entry = (ts_memory_bucket_entry *)ts_array_at(&bucket->buckets, i);
		free(bucket_entry->data);
	}
	ts_array_destroy(&bucket->buckets);
	ts_mutex_unlock(&bucket->bucket_mutex);
	
	ts_mutex_destroy(&bucket->bucket_mutex);
}