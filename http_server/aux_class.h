#ifndef AUX_CLASS
#define AUX_CLASS
#include <memory>
#include <unistd.h>
#include "tool.h"
#include <pthread.h>
class fdwrap
{
	/* 用来包装fd(file descriptor),可以共享fd,自动计数，没有引用时自动释放 */
public:
	explicit fdwrap(int filed) : fd(filed), fdsp(&fd, close) {}
	int read(char *buf, size_t size)
	{
		return ::read(fd, buf, size);
	}
	int write(char *buf, size_t size)
	{
		return ::write(fd, buf, size);
	}
	int getfd()
	{
		return fd;
	}
private:
	int fd;
	std::shared_ptr<int> fdsp;
	static void close(int *fdp)
	{
		if (::close(*fdp) != 0)
			//::err_quit("error when close fd");
			printf("error when close fd\n");
	}
};

class mutexWrap
{
public:
	mutexWrap()
	{
		pthread_mutex_init(&mutex, NULL);
	}
	~mutexWrap()
	{
		pthread_mutex_destroy(&mutex);
	}
	mutexWrap(const mutexWrap&) = delete;
	mutexWrap& operator=(const mutexWrap&) = delete;
	int lock()
	{
		return pthread_mutex_lock(&mutex);
	}
	int unlock()
	{
		return pthread_mutex_unlock(&mutex);
	}
private:
	pthread_mutex_t mutex;
};

class AutoMutex
{
public:
	explicit AutoMutex(mutexWrap &amutex) : mutex(amutex)
	{
		mutex.lock();
	}
	~AutoMutex()
	{
		mutex.unlock();
	}
	AutoMutex(const AutoMutex&) = delete;
	AutoMutex& operator=(const AutoMutex&) = delete;
private:
	mutexWrap &mutex;
};
#endif // !AUX_CLASS

