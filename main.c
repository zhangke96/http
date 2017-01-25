/*************************************************************************
	> File Name: main.c
	> Author: ZhangKe 
	> Mail: zhangke960808@163.com
	> Created Time: Tue 17 Jan 2017 08:47:48 PM CST
 ************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include "tool.h"
#include <errno.h>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
void serve(int sockfd, struct sockaddr *cltaddr, socklen_t *len);

int main(void)
{
    int sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t length;
    char abuf[INET_ADDRSTRLEN];

    bzero(&servaddr, sizeof(servaddr));   
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(80);
//    if ((sefd = (AF_INET, SOCK_STREAM, 0)) < 0)
    if ((sockfd = initserver(SOCK_STREAM, (struct sockaddr *)&servaddr, sizeof(servaddr),128 )) < 0)
        err_quit("error when create the tcp socket");
    // now success create a socket
    for (; ;)
    {
        serve(sockfd, (struct sockaddr *)&clientaddr, &length);
        // print the client address and port number
        printf("client address: %s:%d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, abuf,\
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
    err_sys("failed to initilize the server");
    return -1;
}

void serve(int sockfd, struct sockaddr *clientaddr, socklen_t *length)
{
    int clfd;
    char hello[] = "Hello, World!";
    char buf[128];
    sprintf(buf, "HTTP/1.1 200 OK\r\n\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n\
%s\r\n", sizeof(hello), hello);
    if ((clfd = accept(sockfd, clientaddr, length)) < 0)
    {
        err_sys("accept error");
        return;
    }
    else
    {
        printf("success connect\n");
        send(clfd, buf, strlen(buf), 0);
    }
    close(clfd);
}
