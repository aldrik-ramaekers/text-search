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

#ifndef INCLUDE_UTF8
#define INCLUDE_UTF8

typedef int32_t utf8_char;

typedef struct t_utf8_codepoint
{
	int32_t codepoint;
	u8 data_len;
} utf8_codepoint;

typedef struct t_utf8_str
{
	char* data;
	u32 data_len;
	u32 char_len;
} utf8_str;

typedef struct t_utf8_str_iter
{
	utf8_str iter_target;
	s32 index;
	s32 data_offset;
	utf8_char current_char;
} utf8_str_iter;

utf8_char get_utf8_char_from_codepoint(utf8_codepoint cp);
size_t get_utf8_char_size(utf8_codepoint cp);
utf8_codepoint get_utf8_codepoint_after_offset(utf8_str str, s32 data_offset);
utf8_str_iter iter_utf8_str(utf8_str target);
bool iter_utf8_str_advance(utf8_str_iter *iter);
utf8_str utf8_str_from_buffer(char *data, s32 len);
utf8_str utf8_str_create(char* data);
void utf8_str_print(utf8_str str);

#endif