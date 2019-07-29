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

#define kilobytes(num) num*1000
#define megabytes(num) kilobytes(num*1000)

typedef struct t_memory_bucket
{
	char *data;
	s32 length;
	s32 cursor;
} memory_bucket;

typedef struct t_memory_bucket_collection
{
	mutex bucket_mutex;
	array buckets;
} memory_bucket_collection;

memory_bucket_collection memory_bucket_init(s32 bucket_size);
void* memory_bucket_reserve(memory_bucket_collection *collection, s32 reserve_length);
void memory_bucket_reset(memory_bucket_collection *collection);
void memory_bucket_destroy(memory_bucket_collection *collection);