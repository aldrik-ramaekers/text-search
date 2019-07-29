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

bool thread_tryjoin(thread *thread)
{
	// windows: https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobject
	
	if (thread->valid)
	{
		void *retval;
		bool thread_joined = !pthread_tryjoin_np(thread->thread, &retval);
		return thread_joined;
	}
	return false;
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
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	
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
