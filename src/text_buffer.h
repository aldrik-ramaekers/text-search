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
u8 buffer_done_reading(text_buffer *buffer);

void text_buffer_destroy(text_buffer *buffer);

#endif