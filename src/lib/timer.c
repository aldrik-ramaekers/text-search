/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

float32 timer_elapsed_ms(u64 start)
{
	u64 diff = platform_get_time(TIME_FULL, TIME_US) - start;
	float diff_ms = diff / 1000.0f;
	return diff_ms;
}
