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

#ifndef INCLUDE_ARRAY
#define INCLUDE_ARRAY

typedef struct t_array
{
	u32 length;
	u32 reserved_length;
	u16 entry_size;
	u16 reserve_jump;
	void *data;
	mutex mutex;
} array;

array array_create(u16 entry_size);
int array_push(array *array, void *data);
int array_push_size(array *array, void *data, s32 data_size);
void array_remove_at(array *array, u32 at);
void array_remove(array *array, void *ptr);
void array_remove_by(array *array, void *data);
void *array_at(array *array, u32 at);
void array_destroy(array *array);
void array_swap(array *array, u32 swap1, u32 swap2);
void array_reserve(array *array, u32 reserve_count);
array array_copy(array *array);

#endif