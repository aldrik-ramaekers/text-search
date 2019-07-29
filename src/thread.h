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

#ifdef OS_WINDOWS
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
	CRITICAL_SECTION cs;
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

mutex mutex_create();

void mutex_lock(mutex *mutex);
bool mutex_trylock(mutex *mutex);
void mutex_unlock(mutex *mutex);

void mutex_destroy(mutex *mutex);

#endif