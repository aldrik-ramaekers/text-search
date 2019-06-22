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