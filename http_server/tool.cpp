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

ssize_t readn(int fd, void *ptr, size_t n) // Read "n" bytes from a descriptor. Copy from apue
{
	size_t nleft;
	ssize_t nread;
	nleft = n;
	while (nleft > 0)
	{
		if ((nread = read(fd, ptr, nleft)) < 0)
		{
			if (nleft == n)
				return (-1);  /* error, return -1 */
			else
				break;       /* error, return amount read so far */
		}
		else if (nread == 0)
		{
			break; /* EOF */
		}
		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft); /* return >= 0 */
}

ssize_t writen(int fd, const void *ptr, size_t n) // Write "n" bytes to a descriptor. Copy from apue
{
	size_t nleft;
	ssize_t nwritten;
	nleft = n;
	while (nleft > 0)
	{
		if ((nwritten = write(fd, ptr, nleft)) < 0)
		{
			if (nleft == n)
				return (-1);  /* error, return -1 */
			else
				break;        /* error, return amount written so far */
		}
		else if (nwritten == 0)
		{
			break;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n - nleft);      /* return >= 0 */
}