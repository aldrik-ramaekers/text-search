#include "mutex.h"

ts_mutex ts_mutex_create()
{
	ts_mutex result;
	result.mutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed ts_mutex
	
	return result;
}

ts_mutex ts_mutex_create_recursive()
{
	return ts_mutex_create();
}

void ts_mutex_lock(ts_mutex *ts_mutex)
{
	WaitForSingleObject( 
		ts_mutex->mutex,    // handle to ts_mutex
		INFINITE);  // no time-out interval
}

bool ts_mutex_trylock(ts_mutex *ts_mutex)
{
	return WaitForSingleObject(ts_mutex->mutex, 1) == WAIT_OBJECT_0;
}

void ts_mutex_unlock(ts_mutex *ts_mutex)
{
	ReleaseMutex(ts_mutex->mutex);
}

void ts_mutex_destroy(ts_mutex *ts_mutex)
{
	CloseHandle(ts_mutex->mutex);
}


ts_thread ts_thread_start(void *(*start_routine) (void *), void *arg)
{
	ts_thread result;
	result.valid = 0;
	
	result.thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, 
								 arg, 0, NULL);
	result.valid = 1;
	
	return result;
}

void ts_thread_join(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		WaitForSingleObject(ts_thread->thread, INFINITE);
		CloseHandle(ts_thread->thread);
	}
}

bool ts_thread_tryjoin(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		int result = WaitForSingleObject(ts_thread->thread, 0);
		return result == WAIT_OBJECT_0;
	}
	return 0;
}

void ts_thread_detach(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		CloseHandle(ts_thread->thread);
	}
}

void ts_thread_stop(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		SuspendThread(ts_thread->thread);
	}
}

int ts_thread_get_id()
{
	return GetCurrentThreadId();
}

void ts_thread_exit()
{
	ExitThread(0);
}

void ts_thread_sleep(int microseconds)
{
	Sleep(microseconds/1000);
}