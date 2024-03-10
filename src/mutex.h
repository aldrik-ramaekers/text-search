#ifndef INCLUDE_MUTEX
#define INCLUDE_MUTEX

#if defined(_WIN32)
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

#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

typedef struct t_ts_thread
{
	pthread_t thread;
	bool valid;
} ts_thread;

typedef struct t_ts_mutex
{
	pthread_mutex_t mutex;
} ts_mutex;
#endif

ts_thread 	ts_thread_start(void *(*start_routine) (void *), void *arg);
void 		ts_thread_join(ts_thread *ts_thread);
bool 		ts_thread_tryjoin(ts_thread *ts_thread);
void 		ts_thread_detach(ts_thread *ts_thread);
void 		ts_thread_stop(ts_thread *ts_thread);
int 		ts_thread_get_id();
void 		ts_thread_sleep(int microseconds);
void 		ts_thread_exit();

ts_mutex 	ts_mutex_create_recursive();
ts_mutex 	ts_mutex_create();
void 		ts_mutex_lock(ts_mutex *ts_mutex);
bool 		ts_mutex_trylock(ts_mutex *ts_mutex);
void 		ts_mutex_unlock(ts_mutex *ts_mutex);
void 		ts_mutex_destroy(ts_mutex *ts_mutex);

#endif