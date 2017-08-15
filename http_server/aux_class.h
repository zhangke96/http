#ifndef AUX_CLASS
#define AUX_CLASS
#include <memory>
#include <unistd.h>
#include "tool.h"
#include <pthread.h>
#include <sys/stat.h>
#include "log.h"
class fdwrap
{
	/* ������װfd(file descriptor),���Թ���fd,�Զ�������û������ʱ�Զ��ͷ� */
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
	bool endOfFile() const
	{
		struct stat statinfo;
		if (fstat(fd, &statinfo) != 0)
		{
			printLog(LOG_ERROR, "error when fstat", __FILE__, __LINE__);
			return true;
		}
		off_t fileoffset;
		if ((fileoffset = lseek(fd, 0, SEEK_CUR)) == -1)
		{
			printLog(LOG_ERROR, "can't lseek", __FILE__, __LINE__);
			return true;
		}
		return (fileoffset == statinfo.st_size);
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

