/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

// stop gcc from reporting implicit declaration warning..
extern long int syscall (long int __sysno, ...);
extern int pthread_tryjoin_np(pthread_t thread, void **retval);

thread thread_start(void *(*start_routine) (void *), void *arg)
{
	thread result;
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

inline void thread_detach(thread *thread)
{
	if (thread->valid)
	{
		pthread_detach(thread->thread);
	}
}

inline void thread_join(thread *thread)
{
	if (thread->valid)
	{
		void *retval;
		pthread_join(thread->thread, &retval);
	}
}

inline bool thread_tryjoin(thread *thread)
{
	if (thread->valid)
	{
		void *retval;
		bool thread_joined = !pthread_tryjoin_np(thread->thread, &retval);
		return thread_joined;
	}
	return false;
}

inline void thread_exit()
{
	pthread_exit(0);
}

inline void thread_stop(thread *thread)
{
	if (thread->valid)
	{
		pthread_cancel(thread->thread);
	}
}

mutex mutex_create()
{
	mutex result;
	
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT);
	
	pthread_mutex_init(&result.mutex, &attr);
	
	pthread_mutexattr_destroy(&attr);
	
	return result;
}

mutex mutex_create_recursive()
{
	mutex result;
	
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	
	pthread_mutex_init(&result.mutex, &attr);
	
	pthread_mutexattr_destroy(&attr);
	
	return result;
}

inline void mutex_lock(mutex *mutex)
{
	pthread_mutex_lock(&mutex->mutex);
}

inline bool mutex_trylock(mutex *mutex)
{
	return !pthread_mutex_trylock(&mutex->mutex);
}

inline void mutex_unlock(mutex *mutex)
{
	pthread_mutex_unlock(&mutex->mutex);
}

inline void mutex_destroy(mutex *mutex)
{
	mutex_unlock(mutex);
	pthread_mutex_destroy(&mutex->mutex);
}

inline u32 thread_get_id()
{
	return (u32)syscall(__NR_gettid);
}

inline void thread_sleep(u64 microseconds)
{
	usleep(microseconds);
}
