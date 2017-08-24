/*************************************************************************
	> File Name: tool.h
	> Author:ZhangKe 
	> Mail:ch.zhangke@gmail.com 
	> Created Time: 2016年09月24日 星期六 21时35分39秒
 ************************************************************************/

#ifndef _APUE_H
#define _APUE_H
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include "log.h"
//static mutexWrap mutex;
//#define error_location() \
//    do {\
//        AutoMutex auotmutex(mutex);\
//        printf("error in file %s at line %d\n", __FILE__, __LINE__); \
//    } while(0)
//void err_ret(const char *fmt, ...)
//{
//	AutoMutex auotmutex(mutex);
//    printf("The errno is %s\n", strerror(errno));
//    va_list ap;
//    va_start(ap, fmt);
//    vprintf(fmt, ap);
//    va_end(ap);
//}
//void err_quit(const char *fmt, ...)
//{
//	AutoMutex auotmutex(mutex);
//    printf("The errno is %s\n", strerror(errno));
//    va_list ap;
//    va_start(ap, fmt);
//    vprintf(fmt, ap);
//    va_end(ap);
//    exit(-1);
//}
//void err_sys(const char *fmt, ...)
//{
//	AutoMutex auotmutex(mutex);
//    printf("The errno is %s\n", strerror(errno));
//    va_list ap;
//    va_start(ap, fmt);
//    vprintf(fmt, ap);
//    va_end(ap);
//}
//void err_exit(int err, const char *fmt, ...)
//{
//	AutoMutex auotmutex(mutex);
//    printf("The errno is %s\n", strerror(err));
//    va_list ap;
//    va_start(ap, fmt);
//    vprintf(fmt, ap);
//    va_end(ap);
//    exit(-1);
//}
std::string createNotFound();
std::string createOk();
ssize_t readn(int fd, void *buf, size_t nbytes);  // copy from apue
ssize_t writen(int fd, const void *buf, size_t nbytes); // copy from apue
#endif
