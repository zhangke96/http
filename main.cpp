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

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
void serve(int sockfd, struct sockaddr *cltaddr, socklen_t *len);
void *connectHandThreadFunc(void *);
//int call_message_begin_cb(http_parser *p)
//{
//    printf("call_message_begin\n");
//    return 0;
//}
//int call_header_field_cb(http_parser *p, const char *buf, size_t len)
//{
//    printf("call_header_field %s %zu\n", buf, len);
//    return 0;
//}
//int call_header_value_cb(http_parser *p, const char *buf, size_t len)
//{
//    printf("call_header_value %s %zu\n", buf, len);
//    return 0;
//}
//int call_request_url_cb(http_parser *p, const char *buf, size_t len)
//{
//    printf("call_request_url %s %zu\n", buf, len);
//    return 0;
//}
//int call_body_cb(http_parser *p, const char *buf, size_t len)
//{
//    printf("call_body %s %zu\n", buf, len);
//    return 0;
//}
//int call_headers_complete_cb(http_parser *p)
//{
//    printf("call_headers_complete\n");
//    return 0;
//}
//int call_message_complete_cb(http_parser *p)
//{
//    printf("call_message_complete\n");
//    printf("method: %d\n", p->method);
//    return 0;
//}
//int call_response_status_cb(http_parser *p, const char *buf, size_t len)
//{
//    printf("call_reponse_status %s %zu\n", buf, len);
//    return 0;
//}
//int call_chunk_header_cb(http_parser *p)
//{
//    printf("call_chunk_header\n");
//    return 0;
//}
//int call_chunk_complete_cb(http_parser *p)
//{
//    printf("call_chunk_complete\n");
//    return 0;
//}

/*
 * http server处理过程：
 * 创建服务端套接字，监听指定端口
 * 每一个请求解释http报文，暂时只支持文件的访问，即静态文件服务器
 */
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
    {
        error_location();
        err_quit("error when create the tcp socket");
    }
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
    error_location();
    err_sys("failed to initialize the server");
    return -1;
}

void *
connectHandThreadFunc(void *clfdP)
{
    int clfd = *(int *)clfdP;   //客户端连接ID
    ssize_t recved;
    size_t nparsed, len = 80*1024;
    char abuf[80*1024];
    RequestParser aParser;
    char buf[80*1024];
    char *wp;
    while (!aParser.isFinished())
    {
        recved = recv(clfd, buf, len - 1, 0);
        if (recved < 0) {
            error_location();
            err_sys("error when recv ");
            close(clfd);
            pthread_exit(NULL);
        }
        std::cout << buf << std::endl;
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
        int dfd = open("/home/zhangke/code/http/http", O_RDONLY);
        if (dfd < 0)
        {
            error_location();
            std::cout << "error when open dir" << std::endl;
            pthread_exit(NULL);
        }
//        if (aParser.getUrl() == "/index.html" || aParser.getUrl() == "/")
        {
            std::cout << "/index.html" << std::endl;
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
                pthread_exit(NULL);
            }
            ssize_t readNum;
            ssize_t index = 0;
            wp = abuf;
            while ((readNum = read(sendFd, wp +index, len)) > 0)
            {
                index += readNum;
            }
            sprintf(buf, "HTTP/1.1 200 OK\
Content-Type: text/html\
Content-Length: %ld\r\n\r\n\
%s\r\n", strlen(abuf), abuf);
            send(clfd, buf, strlen(buf), 0);
        }
    }
    close(clfd);
    return NULL;


}
void serve(int sockfd, struct sockaddr *clientaddr, socklen_t *length)
{
    int clfd;
    /*
    char hello[] = "Hello, World!";
    char buf[128];
    char abuf[80*1024];
    size_t nparsed, len = 80*1024;
    ssize_t recved;
    http_parser *parser = (http_parser *)malloc(sizeof(http_parser));
    http_parser_init(parser, HTTP_REQUEST);
    sprintf(buf, "HTTP/1.1 200 OK\
Content-Type: text/html\
Content-Length: %ld\r\n\r\n\
%s\r\n", sizeof(hello), hello);
    */
    if ((clfd = accept(sockfd, clientaddr, length)) < 0)
    {
        error_location();
        err_sys("accept error");
        return;
    }
    else   //开启线程开始接受数据并且处理然后返回结果
    {

        struct sockaddr_in clientaddr;
        char abuf[INET_ADDRSTRLEN];
        printf("client address: %s:%d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, abuf,\
                                                   INET_ADDRSTRLEN), ntohs(clientaddr.sin_port));
        pthread_t pNo;
        int clientfd = clfd;
        pthread_create(&pNo, NULL, connectHandThreadFunc, (void *)&clientfd);
    }
//    else
//    {
//       // http_parser_settings settings;
//        /* set call back function for parse the http request */
//        /*
//        settings.on_message_begin = call_message_begin_cb;
//        settings.on_header_field = call_header_field_cb;
//        settings.on_header_value = call_header_value_cb;
//        settings.on_url = call_request_url_cb;
//        settings.on_status = call_response_status_cb;
//        settings.on_body = call_body_cb;
//        settings.on_headers_complete = call_headers_complete_cb;
//        settings.on_message_complete = call_message_complete_cb;
//        settings.on_chunk_header = call_chunk_header_cb;
//        settings.on_chunk_complete = call_chunk_complete_cb;
//         */
//        recved = recv(clfd, abuf, len, 0);
//        if (recved < 0)
//        {
//            printf("error when recive\n");
//            close(clfd);
//            return;
//        }
//        //        nparsed = http_parser_execute(parser, &settings, abuf, recved);
//        RequestParser aparser;
//        if (aparser.parser_execute(abuf, recved))
//        {
//            if (aparser.getMethod() == HTTP_GET)
//                std::cout << "GET method" << std::endl;
//            std::cout << aparser.getUrl() << std::endl;
//            std::cout << aparser.getValue("User-Agent") << std::endl;
//            send(clfd, buf, strlen(buf), 0);
//        }
//    }
   // close(clfd);
}
