/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_STOPWATCH
#define INCLUDE_STOPWATCH

#if defined(MODE_DEVELOPER) && !defined(MODE_TEST)
s32 _indent_c = 0;
#define debug_print_elapsed_title(_title) printf("%.*s", _indent_c+1, "|---------------------"); printf("%s\n",  _title)
#define debug_print_elapsed_indent() _indent_c+=2;
#define debug_print_elapsed_undent() _indent_c-=2;
#define debug_print_elapsed(_stamp,_title) printf("|%*s%s: %.2fms\n", _indent_c, "", _title, timer_elapsed_ms(_stamp));_stamp = platform_get_time(TIME_FULL, TIME_US);
#else
#define debug_print_elapsed_title(_title) do { } while(0);
#define debug_print_elapsed_indent() do { } while(0);
#define debug_print_elapsed_undent() do { } while(0);
#define debug_print_elapsed(_stamp,_title) do { } while(0);
#endif

float32 timer_elapsed_ms(u64 start);

#endif