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

std::list<HttpConnection> connections;
extern std::ofstream logStream;
int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
int serve(int sockfd, struct sockaddr *cltaddr, socklen_t *len);
void *connectHandThreadFunc(void *);


void sendFile(fdwrap clfd, fdwrap fd, char *buf, size_t bufsize)
{
    ssize_t readSize;
    ssize_t sendSize;
    char sizebuf[15];
    const char terminate[] = "0\r\n\r\n";
    char *work;
    int remainSize = bufsize - 2;
    int index = 0;
    int blockSize = 0;
	int size = 0;
	int status = 0;
    while (true)
    {
		while ((size = fd.read(buf + index, remainSize)) > 0)
		{
			//status = 0;
			blockSize += size;
			index += size;
			remainSize -= size;
		}
		if (size < 0)   // error when read, stop
		{
			printLog(LOG_ERROR, "error when read", __FILE__, __LINE__);
			break;
		}
		snprintf(sizebuf, 14, "%x\r\n", blockSize);
		buf[blockSize] = '\r';
		buf[blockSize + 1] = '\n';
		if ((sendSize = send(clfd.getfd(), sizebuf, strlen(sizebuf), 0)) != strlen(sizebuf))
		{
			printLog(LOG_ERROR, "error when send chunked head", __FILE__, __LINE__);
			pthread_exit(NULL);
		}
		if ((sendSize = send(clfd.getfd(), buf, blockSize + 2, 0)) != blockSize + 2)
		{
			printLog(LOG_ERROR, "error when send chunked body", __FILE__, __LINE__);
			pthread_exit(NULL);
		}
        // resotre the counters
        remainSize = bufsize - 2;
        index = 0;
        blockSize = 0;
		if (size == 0)
		{
			if (fd.endOfFile())
				break;
		}
    }
//    char sendtest[] = "0123456789\r\n";
//    send(clfd, "a\r\n", 3, 0);
//    send(clfd, sendtest, strlen(sendtest), 0);
	printLog(LOG_DEBUG, "I am going to send endding", __FILE__, __LINE__);
    if ((sendSize = send(clfd.getfd(), terminate, strlen(terminate), 0)) != strlen(terminate))
    {
		printLog(LOG_ERROR, "error when send endding, size not equal", __FILE__, __LINE__);
		//close(fd);
        pthread_exit(NULL);
    }
}

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
    servaddr.sin_port        = htons(8000);

    if ((serverfd = initserver(SOCK_STREAM, (struct sockaddr *)&servaddr, sizeof(servaddr),128 )) < 0)
    {
		Log(LOG_ERROR, "error when create the tcp socket " + std::string(strerror(errno)));
		exit(-1);
    }
    // now success create a socket
	std::set<int> shouldDelete;
    for (; ;)
    {
		std::cout << connections.size() << std::endl;
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

void *
connectHandThreadFunc(void *clfdP)
{
    int clfd = *(int *)clfdP;   //客户端连接ID
	fdwrap socketfd(clfd);
    ssize_t recved;
    size_t nparsed, len = 80*1024;
    char abuf[80*1024];
    RequestParser aParser;
    char buf[80*1024];
    char *wp;
    while (!aParser.isFinished())
    {
		printLog(LOG_INFO, "I am recving", __FILE__, __LINE__);
		// use select function to wait for the request from the client
		fd_set rset;
		FD_SET(clfd, &rset);
		timeval waittime = { 10, 0 };
		int selectreturn;
		if ((selectreturn = select(clfd + 1, &rset, NULL, NULL, &waittime)) < 0)
		{
			printLog(LOG_ERROR, "error when select " + std::string(strerror(errno)), __FILE__, __LINE__);
			//close(clfd);
			pthread_exit(NULL);
		}
		else if (selectreturn == 1)
		{
			recved = recv(clfd, buf, len - 1, 0);
			if (recved < 0) {
				printLog(LOG_ERROR, "error when read " + std::string(strerror(errno)), __FILE__, __LINE__);
				//close(clfd);
				pthread_exit(NULL);
			}
		}
		else
		{
			// time out, close the connection
			printLog(LOG_INFO, "time out when select", __FILE__, __LINE__);
			//close(clfd);
			pthread_exit(NULL);
		}
       // std::cout << buf << std::endl;
        wp = buf;
        nparsed = aParser.parser_execute(wp, recved);
        if (nparsed != recved) // 不完整的http报文
        {
            wp = wp + nparsed;
            len = len - 1 - nparsed;
        }
    }
    // 解析完毕 开始处理请求
    if (aParser.getMethod() == HTTP_GET) // get请求
    {
        //打开文件夹
        int dfd = open("./", O_RDONLY);
		fdwrap dfdwrap(dfd);
		//std::unique_ptr<int, decltype(close_fd) *> directory(&dfd, close_fd);
        if (dfd < 0)
        {
			printLog(LOG_ERROR, "error when open dir " + std::string(strerror(errno)), __FILE__, __LINE__);
			//close(clfd);
            pthread_exit(NULL);
        }
		printLog(LOG_INFO, "request file " + aParser.getUrl(), __FILE__, __LINE__);
		int sendFd;
		if (aParser.getUrl() == "/")
			sendFd = open("index.html", O_RDONLY);
		else
			sendFd = open(aParser.getUrl().c_str() + 1, O_RDONLY);
		if (sendFd < 0)
		{
			printLog(LOG_WARN, "failed to open file: " + aParser.getUrl(), __FILE__, __LINE__);
			std::string notFoundHead = createNotFound();
			send(clfd, notFoundHead.c_str(), notFoundHead.size(), 0);
			sendFd = open("404.html", O_RDONLY);
			if (sendFd < 0)
			{
				printLog(LOG_ERROR, "failed to open 404 file", __FILE__, __LINE__);
				pthread_exit(NULL);
			}
			fdwrap sendFdWrap(sendFd);
			sendFile(socketfd, sendFdWrap, buf, 80 * 1024);
			pthread_exit(NULL);
		}
		else
		{
			fdwrap sendFdWrap(sendFd);
			std::string ok = createOk();
			send(clfd, ok.c_str(), ok.size(), 0);
			sendFile(socketfd, sendFdWrap, buf, 80 * 1024);
			//close(sendFd);
		}
    }
   //close(clfd);
    pthread_exit(NULL);
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
		// record the client socket fd
		/*std::string notfound = createNotFound();
		send(clfd, notfound.c_str(), notfound.size(), 0);
		send(clfd, "1\r\n1\r\n0\r\n\r\n", 11, 0);*/
		connections.push_back(HttpConnection(fdwrap(clfd)));
		std::cout << "clfd: " << clfd;
		std::cout << " size: " << connections.size() << std::endl;
		++num;
	}
	if (errno == EAGAIN) // no client connections
	{
		std::cout << "EAGAIN" << std::endl;
		return num;
	}
	else if (clfd == -1)
	{
		Log(LOG_ERROR, "error when accept");
		return num;
	}

}

void *
acceptThread(void *sockfdP)       // thread used to accpet the connection from clients
{
	int clfd;
	struct sockaddr_in clientaddr;
	socklen_t length;
	int sockfd = *reinterpret_cast<int *>(sockfdP);
	char errorstr[100]; // used for strerror_r
	if ((clfd = accept(sockfd, reinterpret_cast<sockaddr *>(&clientaddr), &length) < 0))
	{
		Log(LOG_ERROR, "accept error: " + std::string(strerror_r(errno, errorstr, 100)));
		return NULL;
	}
	else  // add the clfd to the exist connections vector
	{

	}
}