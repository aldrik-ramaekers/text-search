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

#include <synchapi.h>

thread thread_start(void *(*start_routine) (void *), void *arg)
{
	thread result;
	result.valid = false;
	
	result.thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, 
								 arg, 0, NULL);
	result.valid = true;
	
	return result;
}

void thread_join(thread *thread)
{
	if (thread->valid)
	{
		WaitForSingleObject(thread->thread, INFINITE);
		CloseHandle(thread->thread);
	}
}

bool thread_tryjoin(thread *thread)
{
	if (thread->valid)
	{
		s32 result = WaitForSingleObject(thread->thread, 0);
		return result == WAIT_OBJECT_0;
	}
	return false;
}

void thread_detach(thread *thread)
{
	if (thread->valid)
	{
		CloseHandle(thread->thread);
	}
}

void thread_stop(thread *thread)
{
	if (thread->valid)
	{
		SuspendThread(thread->thread);
	}
}

u32 thread_get_id()
{
	return GetCurrentThreadId();
}

void thread_sleep(u64 microseconds)
{
	Sleep(microseconds/1000);
}

mutex mutex_create()
{
	mutex result;
	result.mutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex
	
	return result;
}

void mutex_lock(mutex *mutex)
{
	WaitForSingleObject( 
		mutex->mutex,    // handle to mutex
		INFINITE);  // no time-out interval
}

bool mutex_trylock(mutex *mutex)
{
	return WaitForSingleObject(mutex->mutex, 1) == WAIT_OBJECT_0;
}

void mutex_unlock(mutex *mutex)
{
	ReleaseMutex(mutex->mutex);
}

void mutex_destroy(mutex *mutex)
{
	CloseHandle(mutex->mutex);
}
