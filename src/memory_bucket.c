/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This file is part of Text-search.
*
    *  Text-search is free software: you can redistribute it and/or modify
    *  it under the terms of the GNU General Public License as published by
    *  the Free Software Foundation, either version 3 of the License, or
    *  (at your option) any later version.
	
    *  Text-search is distributed in the hope that it will be useful,
    *  but WITHOUT ANY WARRANTY; without even the implied warranty of
    *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    *  GNU General Public License for more details.
	
    *  You should have received a copy of the GNU General Public License
    *  along with Text-search.  If not, see <https://www.gnu.org/licenses/>.
*/

inline memory_bucket_collection memory_bucket_init(s32 bucket_size)
{
	assert(bucket_size >= MAX_INPUT_LENGTH);
	
	memory_bucket_collection collection;
	collection.bucket_mutex = mutex_create();
	collection.buckets = array_create(sizeof(memory_bucket));
	
	memory_bucket bucket;
	bucket.data = mem_alloc(bucket_size);
	bucket.length = bucket_size;
	bucket.cursor = 0;
	array_push(&collection.buckets, &bucket);
	return collection;
}

void* memory_bucket_reserve(memory_bucket_collection *collection, s32 reserve_length)
{
	mutex_lock(&collection->bucket_mutex);
	memory_bucket *bucket;
	for (s32 i = 0; i < collection->buckets.length; i++)
	{
		bucket = array_at(&collection->buckets, i);
		
		if (bucket->length - bucket->cursor < reserve_length) continue;
		
		void *space = bucket->data+bucket->cursor;
		bucket->cursor += reserve_length;
		mutex_unlock(&collection->bucket_mutex);
		
		return space;
	}
	
	// failed to find suitable space, allocate new bucket
	memory_bucket new_bucket;
	new_bucket.data = mem_alloc(bucket->length);
	new_bucket.length = bucket->length;
	new_bucket.cursor = 0;
	array_push(&collection->buckets, &new_bucket);
	mutex_unlock(&collection->bucket_mutex);
	
	return new_bucket.data;
}

inline void memory_bucket_reset(memory_bucket_collection *collection)
{
	mutex_lock(&collection->bucket_mutex);
	for (s32 i = 0; i < collection->buckets.length; i++)
	{
		memory_bucket *bucket = array_at(&collection->buckets, i);
		bucket->cursor = 0;
	}
	mutex_unlock(&collection->bucket_mutex);
}

inline void memory_bucket_destroy(memory_bucket_collection *collection)
{
	mutex_lock(&collection->bucket_mutex);
	for (s32 i = 0; i < collection->buckets.length; i++)
	{
		memory_bucket *bucket = array_at(&collection->buckets, i);
		mem_free(bucket->data);
	}
	array_destroy(&collection->buckets);
	mutex_unlock(&collection->bucket_mutex);
	
	mutex_destroy(&collection->bucket_mutex);
}