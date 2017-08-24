#ifndef ZK_HTTP_LOG
#define ZK_HTTP_LOG
#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <pthread.h>
enum LogLevel
{
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
};

void printLog(LogLevel level, const std::string& msg, const char *filename, int linenumber);

//use marco to avoid always input __FILE__, __LINE__
#define Log(level, message) \
do {\
	printLog(level, message, __FILE__, __LINE__); \
} while(0)
#endif // !ZK_HTTP_LOG

