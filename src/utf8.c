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

static size_t count_utf8_code_points(const char *s) {
    size_t count = 0;
    while (*s) {
		//printf("%d\n", *s & 0xC0);
        count += (*s++ & 0xC0) != 0x80;
    }
    return count;
}

utf8_str_iter iter_utf8_str(utf8_str target)
{
	utf8_str_iter iter;
	iter.iter_target = target;
	iter.index = 0;
	iter.data_offset = 0;
	return iter;
}

size_t get_utf8_char_size(utf8_codepoint ch)
{
	char first_byte = ch.codepoint & 0xFF;
	
	if (!((first_byte) & 1<<7)) return 1;
	if (!((first_byte) & 1<<5)) return 2;
	if (!((first_byte) & 1<<4)) return 3;
	if (!((first_byte) & 1<<3)) return 4;
	assert(0);
}

utf8_codepoint get_utf8_codepoint_after_offset(utf8_str str, s32 data_offset)
{
	size_t size = get_utf8_char_size(*((utf8_codepoint*)str.data+data_offset));
	
	utf8_codepoint result;
	memset(&result.codepoint, 0, 4);
	memcpy(&result.codepoint, str.data+data_offset, size);
	result.data_len = size;
	
	return result;
}

utf8_char get_utf8_char_from_codepoint(utf8_codepoint cp)
{
	return 0;
}

bool iter_utf8_str_advance(utf8_str_iter *iter)
{
	utf8_codepoint cp = get_utf8_codepoint_after_offset(iter->iter_target, 
														iter->data_offset);
	iter->data_offset += cp.data_len;
	
	if (iter->iter_target.data_len < iter->data_offset) return false;
	iter->index++;
	
	printf("%x\n", cp.codepoint);
	iter->current_char = get_utf8_char_from_codepoint(cp);
	
	return true;
}

utf8_str utf8_str_from_buffer(char *data, s32 len)
{
	utf8_str s;
	s.char_len = count_utf8_code_points(data);
	s.data_len = len;
	s.data = data;
	return s;
}

utf8_str utf8_str_create(char *data)
{
	utf8_str s;
	s.char_len = count_utf8_code_points(data);
	s.data_len = strlen(data);
	s.data = mem_alloc(s.data_len);
	memcpy(s.data, data, s.data_len);
	return s;
}

void utf8_str_print(utf8_str str)
{
	printf("length: %d, data size: %d\n", str.char_len, str.data_len);
}
