#ifndef INCLUDE_MUTEX
#define INCLUDE_MUTEX

#ifdef _WIN32
#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>
#include <synchapi.h>

typedef struct t_ts_mutex
{
	HANDLE mutex;
} ts_mutex;

typedef struct t_ts_thread
{
	HANDLE thread;
	int valid;
} ts_thread;
#endif

ts_thread 	ts_thread_start(void *(*start_routine) (void *), void *arg);
void 		ts_thread_join(ts_thread *ts_thread);
int 		ts_thread_tryjoin(ts_thread *ts_thread);
void 		ts_thread_detach(ts_thread *ts_thread);
void 		ts_thread_stop(ts_thread *ts_thread);
int 		ts_thread_get_id();
void 		ts_thread_sleep(int microseconds);
void 		ts_thread_exit();

ts_mutex 	ts_mutex_create_recursive();
ts_mutex 	ts_mutex_create();
void 		ts_mutex_lock(ts_mutex *ts_mutex);
int 		ts_mutex_trylock(ts_mutex *ts_mutex);
void 		ts_mutex_unlock(ts_mutex *ts_mutex);
void 		ts_mutex_destroy(ts_mutex *ts_mutex);

#endif