#ifndef INCLUDE_PROFILER
#define INCLUDE_PROFILER

typedef struct t_profiler_result
{
	u64 sample_count;
	u64 total_ns;
	char *name;
	char *file;
	s32 line_nr;
} profiler_result;

typedef struct t_profiler
{
	array results;
} profiler;

// TODO(Aldrik): profiler name and file name are not freed

u64 profiler_begin_d();
void profiler_end_d(u64 begin_stamp, const char *function_name, const char *file_name, s32 line_nr);
void profiler_create();
void profiler_destroy();
void profiler_update_render(platform_window *window, s32 x, s32 y,s32 w, s32 h, camera *camera, u8 render, mouse_input *mouse);

mutex profiler_mutex;

#define profiler_begin(identifier__) u64 identifier__ = profiler_begin_d();
#define profiler_end(begin_stamp__) profiler_end_d(begin_stamp__, __FUNCTION__, __FILE__, __LINE__)

#endif