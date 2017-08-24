/*************************************************************************
	> File Name: main.c
	> Author: ZhangKe
	> Mail: zhangke960808@163.com
	> Created Time: Tue 17 Jan 2017 08:47:48 PM CST
 ************************************************************************/

#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include "tool.h"
#include <errno.h>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "http_parser.h"
#include "RequestParser.h"
#include <time.h>
#include <sys/select.h>
#include <memory>
#include <vector>
#include "aux_class.h"
#include "log.h"
#include <signal.h>
#include <list>
#include "HttpConnection.h"
#include <set>

static std::list<HttpConnection> connections;
extern std::ofstream logStream;
int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
int serve(int sockfd, struct sockaddr *cltaddr, socklen_t *len);

/*
 * http server处理过程：
 * 创建服务端套接字，监听指定端口
 * 每一个请求解释http报文，暂时只支持文件的访问，即静态文件服务器
 */

int main(void)
{
	logStream.open("log", std::ostream::app);
	/* ignore signal SIGPIPE */
	sigset_t signals;
	if (sigemptyset(&signals) != 0)
	{
		Log(LOG_ERROR, "error when empty the signal set");
		exit(1);
	}
	if (sigaddset(&signals, SIGPIPE) != 0)
	{
		Log(LOG_ERROR, "error when add signal SIGPIPE");
		exit(1);
	}
	if (sigprocmask(SIG_BLOCK, &signals, nullptr) != 0)
	{
		Log(LOG_ERROR, "error when block the signal");
		exit(1);
	}
    int serverfd;
    struct sockaddr_in servaddr;
    socklen_t length;
    char abuf[INET_ADDRSTRLEN];

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(80);

    if ((serverfd = initserver(SOCK_STREAM, (struct sockaddr *)&servaddr, sizeof(servaddr),128 )) < 0)
    {
		Log(LOG_ERROR, "error when create the tcp socket " + std::string(strerror(errno)));
		exit(-1);
    }
    // now success create a socket
	std::set<int> shouldDelete;
    for (; ;)
    {
		shouldDelete.clear();
        struct sockaddr_in clientaddr;
		length = sizeof(clientaddr);
		int maxfd = serverfd;
		fd_set rd, wrt;
		FD_ZERO(&rd);
		FD_ZERO(&wrt);
		FD_SET(serverfd, &rd);
		// add all clients' connection register
		for (auto &connect : connections)
		{
			if (connect.state == Http_Connection_Receiving)
				FD_SET(connect.getfd(), &rd);
			else
				FD_SET(connect.getfd(), &wrt);
			if (connect.getfd() > maxfd)
				maxfd = connect.getfd();
		}
		int selectreturn;
		Log(LOG_INFO, "max fd is: " + std::to_string(maxfd));
		
		if ((selectreturn = select(maxfd + 1, &rd, &wrt, NULL, NULL)) < 0)  // wait for the connection from clients
		{
			Log(LOG_ERROR, "error when select" + std::string(strerror(errno)));
			exit(0);
		}
		else
		{
			for (auto &connect : connections)
			{
				if (FD_ISSET(connect.getfd(), &rd))
				{
					if (connect.state == Http_Connection_Receiving)
					{
						Log(LOG_INFO, "receve from client");
						int size = connect.read();
						if (size < 0)
						{
							Log(LOG_ERROR, "error when read from client");
							shouldDelete.insert(connect.getfd());
						}
					}
				}
				if (FD_ISSET(connect.getfd(), &wrt))
				{
					if (connect.state != Http_Connection_Receiving)
					{
						Log(LOG_INFO, "write to the client");
						int size = connect.write();
						if (size < 0)
						{
							Log(LOG_ERROR, "error when write");
							shouldDelete.insert(connect.getfd());
						}
						if (connect.sendAllBody())
						{
							shouldDelete.insert(connect.getfd());
						}
					}
				}
			}
			for (auto dele : shouldDelete)
			{
				connections.remove_if([dele](HttpConnection &connect) { return connect.getfd() == dele; });
			}
			if (FD_ISSET(serverfd, &rd))  // server socket can accept the connections from clients
			{
				int num;
				if ((num = serve(serverfd, reinterpret_cast<struct sockaddr *>(&clientaddr), &length)) < 0)
				{
					Log(LOG_ERROR, "serve error");
					exit(0);
				}
				else
				{
					Log(LOG_INFO, "get " + std::to_string(num) + " connections");
				}
			}
		}
    }
    return 0;
}

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
    /*
     *  create a server socket
     */
    int fd;
    int err = 0;
    int reuse = 1;
	if ((fd = socket(addr->sa_family, type, 0)) < 0)
	{
		Log(LOG_ERROR, "error when create server socket");
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
		sizeof(int)) < 0)
	{
		Log(LOG_ERROR, "error when set socket reuse");
		goto errout;
	}
	int val;
	if ((val = fcntl(fd, F_GETFL, 0)) == -1)
	{
		Log(LOG_ERROR, "error when get fcntl");
		goto errout;
	}
	if ((val = fcntl(fd, F_SETFL, val | O_NONBLOCK)) == -1)
	{
		Log(LOG_ERROR, "error when set noblock flag");
		goto errout;
	}
	if (bind(fd, addr, alen) < 0)  /* bind socket fd to address addr */
	{
		Log(LOG_ERROR, "error when bind address");
		goto errout;
	}
	if (listen(fd, qlen) < 0)
	{
		Log(LOG_ERROR, "error when listen");
		goto errout;
	}
    return fd;

errout:
    err = errno;
    close(fd);
    errno = err;
	Log(LOG_ERROR, "failed to initialize the server " + std::string(strerror(errno)));
    return -1;
}

int serve(int sockfd, struct sockaddr *clientaddr, socklen_t *length)
{
    int clfd;
	int val;
	int num = 0;
	if ((val = fcntl(sockfd, F_GETFL, 0)) == -1)
	{
		Log(LOG_ERROR, "fcntl error");
		return -1;
	}
	if (val & O_NONBLOCK)
	{
	}
	else
	{
		Log(LOG_ERROR, "can only work with noblocking socket");
		return -1;
	}
	while ((clfd = accept(sockfd, clientaddr, length)) >= 0)
	{
		char abuf[INET_ADDRSTRLEN];
		Log(LOG_INFO, "client address: " + std::string(inet_ntop(AF_INET, &((reinterpret_cast<struct sockaddr_in *>(clientaddr))->sin_addr), abuf, \
			INET_ADDRSTRLEN)) + ":" + std::to_string(ntohs((reinterpret_cast<struct sockaddr_in *>(clientaddr))->sin_port)));
		connections.push_back(HttpConnection(fdwrap(clfd)));
		++num;
	}
	if (errno == EAGAIN) // no client connections
	{
		return num;
	}
	else if (clfd == -1)
	{
		Log(LOG_ERROR, "error when accept");
		return num;
	}

}
