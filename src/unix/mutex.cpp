#include "mutex.h"

ts_thread ts_thread_start(void *(*start_routine) (void *), void *arg)
{
	ts_thread result;
	result.valid = false;
	
	pthread_attr_t attr;
	int attr_init_result = pthread_attr_init(&attr);
	if (attr_init_result)
		return result;
	
	int start_thread_result = pthread_create(&result.thread, &attr, start_routine, arg);
	if (start_thread_result)
	{
		pthread_attr_destroy(&attr);
		return result;
	}
	
	result.valid = true;
	pthread_attr_destroy(&attr);
	
	return result;
}

void ts_thread_detach(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		pthread_detach(ts_thread->thread);
	}
}

void ts_thread_join(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		void *retval;
		pthread_join(ts_thread->thread, &retval);
	}
}

bool ts_thread_tryjoin(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		void *retval;
		bool thread_joined = !pthread_join(ts_thread->thread, &retval);
		return thread_joined;
	}
	return false;
}

void ts_thread_exit()
{
	pthread_exit(0);
}

void ts_thread_stop(ts_thread *ts_thread)
{
	if (ts_thread->valid)
	{
		pthread_cancel(ts_thread->thread);
	}
}

ts_mutex ts_mutex_create()
{
	ts_mutex result;
	
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT);
	
	pthread_mutex_init(&result.mutex, &attr);
	
	pthread_mutexattr_destroy(&attr);
	
	return result;
}

ts_mutex ts_mutex_create_recursive()
{
	ts_mutex result;
	
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	
	pthread_mutex_init(&result.mutex, &attr);
	
	pthread_mutexattr_destroy(&attr);
	
	return result;
}

void ts_mutex_lock(ts_mutex *ts_mutex)
{
	pthread_mutex_lock(&ts_mutex->mutex);
}

bool ts_mutex_trylock(ts_mutex *ts_mutex)
{
	return !pthread_mutex_trylock(&ts_mutex->mutex);
}

void ts_mutex_unlock(ts_mutex *ts_mutex)
{
	pthread_mutex_unlock(&ts_mutex->mutex);
}

void ts_mutex_destroy(ts_mutex *ts_mutex)
{
	ts_mutex_unlock(ts_mutex);
	pthread_mutex_destroy(&ts_mutex->mutex);
}

void ts_thread_sleep(int microseconds)
{
	usleep(microseconds);
}