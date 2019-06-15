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
	Sleep(microseconds*1000);
}

mutex mutex_create()
{
	mutex result;
	result.mutex = CreateMutexA(NULL, TRUE, NULL);
	return result;
}

void mutex_lock(mutex *mutex)
{
	WaitForSingleObject(mutex->mutex, INFINITE);
}

bool mutex_trylock(mutex *mutex)
{
	s32 result = WaitForSingleObject(mutex->mutex, 0);
	return result == WAIT_OBJECT_0;
}

void mutex_unlock(mutex *mutex)
{
	ReleaseMutex(mutex->mutex);
}

void mutex_destroy(mutex *mutex)
{
	CloseHandle(mutex->mutex);
}
