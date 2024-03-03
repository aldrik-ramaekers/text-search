#include "mutex.h"

mutex mutex_create()
{
	mutex result;
	result.mutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex
	
	return result;
}

mutex mutex_create_recursive()
{
	return mutex_create();
}

void mutex_lock(mutex *mutex)
{
	WaitForSingleObject( 
		mutex->mutex,    // handle to mutex
		INFINITE);  // no time-out interval
}

int mutex_trylock(mutex *mutex)
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


thread thread_start(void *(*start_routine) (void *), void *arg)
{
	thread result;
	result.valid = 0;
	
	result.thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, 
								 arg, 0, NULL);
	result.valid = 1;
	
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

int thread_tryjoin(thread *thread)
{
	if (thread->valid)
	{
		int result = WaitForSingleObject(thread->thread, 0);
		return result == WAIT_OBJECT_0;
	}
	return 0;
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

int thread_get_id()
{
	return GetCurrentThreadId();
}

void thread_exit()
{
	ExitThread(0);
}

void thread_sleep(int microseconds)
{
	Sleep(microseconds/1000);
}