#include "tool.h"
std::string createNotFound()
{
	time_t nowTime = time(NULL);
	struct tm tmTemp;
	const struct tm *toParse = gmtime_r(&nowTime, &tmTemp);
	char timebuf[40];
	if (strftime(timebuf, 40, "Date: %a, %d %b %G %T %Z", toParse) == 0)
	{
		printLog(LOG_WARN, "time buf is too short", __FILE__, __LINE__);
	}
	std::string response("HTTP/1.1 404 Not Found\r\n"
		"Server: zhangke/0.1\r\n");
	response.append(timebuf);
	response.append("\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\nConnection:close\r\n\r\n");
	return response;
}
std::string createOk()
{
	time_t nowTime = time(NULL);
	struct tm tmTemp;
	const struct tm *toParse = gmtime_r(&nowTime, &tmTemp);
	char timebuf[40];
	if (strftime(timebuf, 40, "Date: %a, %d %b %G %T %Z", toParse) == 0)
	{
		printLog(LOG_WARN, "time buf is too short", __FILE__, __LINE__);
	}
	std::string response("HTTP/1.1 200 OK\r\n"
		"Server: zhangke/0.1\r\n");
	response.append(timebuf);
	response.append("\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\nConnection:close\r\n\r\n");
	return response;
}