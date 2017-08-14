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

//std::vector<std::string, __gnu_cxx::pool_allocator<std::string>> vec;

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
void serve(int sockfd, struct sockaddr *cltaddr, socklen_t *len);
void *connectHandThreadFunc(void *);
std::string createNotFound()
{
    time_t nowTime = time(NULL);
	struct tm tmTemp;
    const struct tm *toParse = gmtime_r(&nowTime, &tmTemp);
    char timebuf[40];
    if (strftime(timebuf, 40, "Date: %a, %d %b %G %T %Z", toParse) == 0)
    {
        error_location();
        std::cout << "timebuf 40 is to short" << std::endl;
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
        error_location();
        std::cout << "timebuf 40 is to short" << std::endl;
    }
    std::string response("HTTP/1.1 200 OK\r\n"
                                 "Server: zhangke/0.1\r\n");
    response.append(timebuf);
    response.append("\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\nConnection:close\r\n\r\n");
    return response;
}
void sendFile(fdwrap clfd, fdwrap fd, char *buf, size_t bufsize)
{
    ssize_t readSize;
    ssize_t sendSize;
    char sizebuf[15];
    const char terminate[] = "0\r\n\r\n";
    char *work;
    int status = 1;
    int remainSize = bufsize - 2;
    int index = 0;
    size_t alineSize;
    int blockSize = 0;
    FILE *fileInput = fdopen(fd.getfd(), "r");
    while (status != 2)
    {
        /*
        while ((work = fgets(buf + index, remainSize, fileInput)) != NULL)   // fgets will store \n and change to \r\n
        {
            status = 0;
            alineSize = strlen(work);
            if (alineSize > 1)
            {
                if (work[alineSize - 1] == '\n' && work[alineSize - 2] != '\r')
                {
                    work[alineSize - 1] = '\r';
                    work[alineSize] = '\n';
                }
                else
                    --alineSize;
            }
            else
            {
                work[alineSize - 1] = '\r';
                work[alineSize] = '\n';
            }
            blockSize += (alineSize - 1);
            index += (alineSize + 2);
            remainSize -= (alineSize + 2);
        }
         */
        while ((work = fgets(buf + index, remainSize, fileInput)) != NULL)   // fgets will store \n and change to \r\n
        {
            status = 0;
            alineSize = strlen(work);
            blockSize += alineSize;
            index += alineSize;
            remainSize -= alineSize;
        }
        // read a block, send
        if (status == 0)
        {
            snprintf(sizebuf, 14, "%x\r\n", blockSize);
            printf("a block size:%s", sizebuf);
            buf[blockSize] = '\r';
            buf[blockSize+1] = '\n';
            if ((sendSize = send(clfd.getfd(), sizebuf, strlen(sizebuf), 0)) != strlen(sizebuf))
            {
                error_location();
				//close(fd);
                pthread_exit(NULL);
            }
            if ((sendSize = send(clfd.getfd(), buf, blockSize + 2, 0)) != blockSize + 2)
            {
                error_location();
				//close(fd);
                pthread_exit(NULL);
            }
        }
        // resotre the counters
        remainSize = bufsize - 2;
        index = 0;
        blockSize = 0;
        ++status;   // status == 2 can confirm I have read all the file
        if (status == 2)
        {
            std::cout << "I have read all the 404 file" << std::endl;
        }
    }
//    char sendtest[] = "0123456789\r\n";
//    send(clfd, "a\r\n", 3, 0);
//    send(clfd, sendtest, strlen(sendtest), 0);
    std::cout << "I am going to send endding" << std::endl;
    if ((sendSize = send(clfd.getfd(), terminate, strlen(terminate), 0)) != strlen(terminate))
    {
        error_location();
        printf("send endding\n");
		//close(fd);
        pthread_exit(NULL);
    }
    std::cout << "terminate lenght: " << strlen(terminate) << std::endl;
    std::cout << "sending all" << std::endl;
    /*
    while ((readSize = read(fd, buf, bufsize)) > 0)
    {
        snprintf(sizebuf, 14, "\r\n%x\r\n", readSize);
        if ((sendSize = send(clfd, sizebuf, strlen(sizebuf), 0)) != strlen(sizebuf))
        {
            error_location();
            pthread_exit(NULL);
        }
        printf("%s", sizebuf);
        if ((sendSize = send(clfd, buf, readSize, 0)) != readSize)
        {
            error_location();
            pthread_exit(NULL);
        }
        printf("%s", buf);
    }
    if ((sendSize = send(clfd, terminate, strlen(terminate), 0)) != strlen(terminate))
    {
        error_location();
        pthread_exit(NULL);
    }
    printf("%s", terminate);
     */
}

/*
 * http server处理过程：
 * 创建服务端套接字，监听指定端口
 * 每一个请求解释http报文，暂时只支持文件的访问，即静态文件服务器
 */
int main(void)
{
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t length;
    char abuf[INET_ADDRSTRLEN];

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(80);

    if ((sockfd = initserver(SOCK_STREAM, (struct sockaddr *)&servaddr, sizeof(servaddr),128 )) < 0)
    {
        error_location();
        err_quit("error when create the tcp socket");
    }
    // now success create a socket
    for (; ;)
    {
        struct sockaddr_in clientaddr;
        serve(sockfd, (struct sockaddr *)&clientaddr, &length);
        // print the client address and port number
   //     printf("client address: %s:%d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, abuf,\
                                                   INET_ADDRSTRLEN), ntohs(clientaddr.sin_port));
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
        return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                  sizeof(int)) < 0)
        goto errout;
    if (bind(fd, addr, alen) < 0)  /* bind socket fd to address addr */
        goto errout;
    if (listen(fd, qlen) < 0)
        goto errout;

    return fd;

errout:
    err = errno;
    close(fd);
    errno = err;
    error_location();
    err_sys("failed to initialize the server");
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
        std::cout << "I am recving" << std::endl;
		// use select function to wait for the request from the client
		fd_set rset;
		FD_SET(clfd, &rset);
		timeval waittime = { 10, 0 };
		int selectreturn;
		if ((selectreturn = select(clfd + 1, &rset, NULL, NULL, &waittime)) < 0)
		{
			error_location();
			err_sys("error when recv ");
			//close(clfd);
			pthread_exit(NULL);
		}
		else if (selectreturn == 1)
		{
			recved = recv(clfd, buf, len - 1, 0);
			if (recved < 0) {
				error_location();
				err_sys("error when recv ");
				//close(clfd);
				pthread_exit(NULL);
			}
		}
		else
		{
			// time out, close the connection
			error_location();
			err_sys("error when wait for the request from client, time out ");
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
            error_location();
            std::cout << "error when open dir" << std::endl;
			//close(clfd);
            pthread_exit(NULL);
        }
//        if (aParser.getUrl() == "/index.html" || aParser.getUrl() == "/")
        {
            printf("request file: %s\n", aParser.getUrl().c_str() + 1);
            int sendFd;
            if (aParser.getUrl() == "/")
                sendFd = open("index.html", O_RDONLY);
            else
                sendFd = open(aParser.getUrl().c_str() + 1, O_RDONLY);
            if (sendFd < 0)
            {
                error_location();
                perror("open file to send");
                std::string notFoundHead = createNotFound();
                send(clfd, notFoundHead.c_str(), notFoundHead.size(), 0);
                sendFd = open("404.html", O_RDONLY);
                if (sendFd < 0)
                {
                    error_location();
                    pthread_exit(NULL);
                }
				fdwrap sendFdWrap(sendFd);
                sendFile(socketfd, sendFdWrap, buf, 80*1024);
                pthread_exit(NULL);
            }
            else
            {
				fdwrap sendFdWrap(sendFd);
                std::string ok = createOk();
                send(clfd, ok.c_str(), ok.size(), 0);
                sendFile(socketfd, sendFdWrap, buf, 80*1024);
				//close(sendFd);
            }
        }
    }
   //close(clfd);
    pthread_exit(NULL);
}
void serve(int sockfd, struct sockaddr *clientaddr, socklen_t *length)
{
    int clfd;
    if ((clfd = accept(sockfd, clientaddr, length)) < 0)
    {	
        error_location();
        err_sys("accept error");
        return;
    }
    else   //开启线程开始接受数据并且处理然后返回结果
    {
        char abuf[INET_ADDRSTRLEN];
        printf("client address: %s:%d\n", inet_ntop(AF_INET, &((reinterpret_cast<struct sockaddr_in *>(clientaddr))->sin_addr), abuf,\
                                                   INET_ADDRSTRLEN), ntohs((reinterpret_cast<struct sockaddr_in *>(clientaddr))->sin_port));
        pthread_t pNo;
        int clientfd = clfd;
        pthread_create(&pNo, NULL, connectHandThreadFunc, (void *)&clientfd);
    }
}
