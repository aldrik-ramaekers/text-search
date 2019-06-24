#ifndef INCLUDE_FILE
#define INCLUDE_FILE

typedef struct t_text_buffer
{
	char *data;
	s32 len;
	s32 buffer_size;
	s32 read_cursor;
} text_buffer;

text_buffer text_buffer_create(s32 buffer_size);

void buffer_write_signed(text_buffer *buffer, s64 val);
void buffer_write_unsigned(text_buffer *buffer, u64 val);
void buffer_write_string(text_buffer *buffer, char *string);

s64 buffer_read_signed(text_buffer *buffer);
u64 buffer_read_unsigned(text_buffer *buffer);
char *buffer_read_string(text_buffer *buffer, char *string_buffer);

void text_buffer_destroy(text_buffer *buffer);

#endif