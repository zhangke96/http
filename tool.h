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
void err_ret(const char *str)
{
    printf("%s; The errno is %s\n",str, strerror(errno));
}
void err_quit(const char *fmt, ...)
{
    printf("The errno is %s\n", strerror(errno));
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    exit(-1);
}
void err_sys(const char *fmt, ...)
{
    printf("The errno is %s\n", strerror(errno));
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
void err_exit(int err, const char *fmt, ...)
{
    printf("The errno is %s\n", strerror(err));
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    exit(-1);
}
#endif
