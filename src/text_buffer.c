
text_buffer text_buffer_create(s32 buffer_size)
{
	text_buffer tb;
	tb.data = mem_alloc(buffer_size);
	tb.data[0] = 0;
	tb.len = 0;
	tb.buffer_size = buffer_size;
	tb.read_cursor = 0;
	
	return tb;
}

void buffer_write_signed(text_buffer *buffer, s64 val)
{
	assert(buffer);
	assert(buffer->buffer_size-buffer->len > 32);
	
	char nr_buf[32];
	snprintf(nr_buf, 32, "%"PRId64"\n", val);
	s32 len = strlen(nr_buf);
	memcpy(buffer->data+buffer->len, nr_buf, len);
	buffer->len += len;
	buffer->data[buffer->len] = 0;
}

void buffer_write_unsigned(text_buffer *buffer, u64 val)
{
	assert(buffer);
	assert(buffer->buffer_size-buffer->len > 32);
	
	char nr_buf[32];
	snprintf(nr_buf, 32, "%"PRIu64"\n", val);
	s32 len = strlen(nr_buf);
	memcpy(buffer->data+buffer->len, nr_buf, len);
	buffer->len += len;
	buffer->data[buffer->len] = 0;
}

void buffer_write_string(text_buffer *buffer, char *string)
{
	assert(buffer);
	assert(string);
	s32 len = strlen(string);
	assert(buffer->buffer_size-buffer->len > len);
	
	memcpy(buffer->data+buffer->len, string, len);
	buffer->len += len;
	buffer->data[buffer->len] = '\n';
	buffer->len += 1;
	buffer->data[buffer->len] = 0;
}

s64 buffer_read_signed(text_buffer *buffer)
{
	char val[32];
	char *data = buffer->data+buffer->read_cursor;
	
	s32 index = 0;
	while (*data != '\n')
	{
		val[index++] = *data;
		++data;
	}
	val[index] = 0;
	buffer->read_cursor += index+1;
	
	return string_to_s64(val);
}

u64 buffer_read_unsigned(text_buffer *buffer)
{
	char val[32];
	char *data = buffer->data+buffer->read_cursor;
	
	s32 index = 0;
	while (*data != '\n')
	{
		val[index++] = *data;
		++data;
	}
	val[index] = 0;
	buffer->read_cursor += index+1;
	
	return string_to_u64(val);
}

u8 buffer_done_reading(text_buffer *buffer)
{
	return buffer->data[buffer->read_cursor] == 0;
}

char *buffer_read_string(text_buffer *buffer, char *string_buffer)
{
	char *data = buffer->data+buffer->read_cursor;
	
	s32 index = 0;
	while (*data != '\n')
	{
		index++;
		++data;
	}
	char ch = *data;
	*data = 0;
	strncpy(string_buffer, buffer->data+buffer->read_cursor, MAX_INPUT_LENGTH);
	*data = ch;
	
	buffer->read_cursor += index+1;
	
	return string_buffer;
}

void text_buffer_destroy(text_buffer *buffer)
{
	assert(buffer);
	mem_free(buffer->data);
}