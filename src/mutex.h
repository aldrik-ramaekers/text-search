#ifndef INCLUDE_MUTEX
#define INCLUDE_MUTEX

#ifdef _WIN32
#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>
#include <synchapi.h>

typedef struct t_mutex
{
	HANDLE mutex;
} mutex;

typedef struct t_thread
{
	HANDLE thread;
	int valid;
} thread;
#endif

thread thread_start(void *(*start_routine) (void *), void *arg);
void thread_join(thread *thread);
int thread_tryjoin(thread *thread);
void thread_detach(thread *thread);
void thread_stop(thread *thread);
int thread_get_id();
void thread_sleep(int microseconds);
void thread_exit();


mutex mutex_create_recursive();
mutex mutex_create();
void mutex_lock(mutex *mutex);
int mutex_trylock(mutex *mutex);
void mutex_unlock(mutex *mutex);
void mutex_destroy(mutex *mutex);


#endif