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
static std::ofstream logStream;
std::ofstream& printLog(LogLevel level, const std::string& msg, const char *filename, int linenumber)
{
	struct timeval nowtime;
	gettimeofday(&nowtime, nullptr);
	struct tm timestruct;
	struct tm *timep = nullptr;
	if ((timep = gmtime_r(&(nowtime.tv_sec), &timestruct)) == nullptr)
	{
		std::cerr << "error when convernt time" << std::endl; // hope never happen
	}
	// format is 20170815 15:06:24.125878 
	logStream << timep->tm_year + 1900 << ((timep->tm_mon + 1 > 10) ? "" : "0")
		<< timep->tm_mon + 1 << ((timep->tm_mday > 10) ? "" : "0") << timep->tm_mday
		<< " " << ((timep->tm_hour > 10) ? "" : "0") << timep->tm_hour
		<< ":" << ((timep->tm_min > 10) ? "" : "0") << timep->tm_min
		<< ":" << ((timep->tm_sec > 10) ? "" : "0") << timep->tm_sec
		<< "." << nowtime.tv_usec << " ";
	logStream << (unsigned long)pthread_self() << " ";
	if (level == LOG_DEBUG)
		logStream << "DEBUG";
	else if (level == LOG_INFO)
		logStream << "INFO";
	else if (level == LOG_WARN)
		logStream << "WARN";
	else if (level == LOG_ERROR)
		logStream << "ERROR";
	else
		logStream << "UNRECOGNIZE";
	logStream << " " << msg << " " << filename << ":" << linenumber << std::endl;
	return logStream;
}
#endif // !ZK_HTTP_LOG

