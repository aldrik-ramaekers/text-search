/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_THREAD
#define INCLUDE_THREAD

#ifdef OS_LINUX
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

struct t_thread
{
	pthread_t thread;
	bool valid;
};

struct t_mutex
{
	pthread_mutex_t mutex;
};
#endif

#ifdef OS_WIN
#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>

struct t_thread
{
	HANDLE thread;
	bool valid;
};

struct t_mutex
{
	HANDLE mutex;
};
#endif

typedef struct t_thread thread;
typedef struct t_mutex mutex;

thread thread_start(void *(*start_routine) (void *), void *arg);
void thread_join(thread *thread);
bool thread_tryjoin(thread *thread);
void thread_detach(thread *thread);
void thread_stop(thread *thread);
u32 thread_get_id();
void thread_sleep(u64 microseconds);

mutex mutex_create_recursive();
mutex mutex_create();

void mutex_lock(mutex *mutex);
bool mutex_trylock(mutex *mutex);
void mutex_unlock(mutex *mutex);

void mutex_destroy(mutex *mutex);

#endif