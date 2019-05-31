#ifndef INCLUDE_SYSTEM_WATCH
#define INCLUDE_SYSTEM_WATCH

typedef struct t_system_info
{
	s32 cpu_count;
	s32 memory_size;
	cpu_info cpu;
} system_info;

void system_watch_update_render(platform_window *window, camera *camera, s32 x, s32 y, s32 w, s32 h);

#endif