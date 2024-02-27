#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cbdefine.h"

// MsgLevel与color的元素个数必须对应，否则将会触发段错误
enum MsgLevel
{
    ML_ERR = 0,            // 错误            错误信息，正常运行绝对不应该出现的打印
    ML_WARN = 1,           // 警告            低警告级别，可能会对监控带来部分影响的
    ML_WARN_2 = 2,         // 警告2           高警告级别，肯定会对监控带来部分影响的
    ML_INFO,               // 通用信息
    ML_INFO_PROC,          // 进程监控相关
    ML_INFO_FILE,          // 文件监控相关
    ML_INFO_EVENT,         // 进程事件
};

static char *color[] = {
    "\033[1;31m",            // 亮红
//    "\033[1;31;5m",            // 亮红闪
    //    "\033[0;32;31m",         // 暗红

    "\033[0;33m",            // 棕色
    "\033[1;33m",            // 亮黄
//    "\033[0;33;5m",            // 棕色闪
//    "\033[1;33;5m",            // 亮黄闪

    "\033[1;37m",            // 白
    "\033[1;36m",            // 青蓝
    //    "\033[1;34m",            // 亮蓝
    //    "\033[0;32;34m",         // 暗蓝
    "\033[1;32m",            // 亮绿
    //    "\033[0;32;32m",         // 暗绿
    "\033[1;35m",            // 紫色
};

#define mdebug 0
#if mdebug
#define dmsg(fmt, ...)\
{\
    printf("%llu ",gettid());\
    printf(fmt,##__VA_ARGS__);\
}
#else
#define dmsg(fmt, ...)
#endif

#define SPIK_LEVEN(level) if(/*level > ML_INFO*/0) break;
#define NCOLOR              "\033[m"    /* 清除颜色 */
#define DMSG(level, fmt, ...)\
({\
    do{\
        SPIK_LEVEN(level);\
        printf(color[level]);\
        struct timespec timestamp;\
        clock_gettime(CLOCK_REALTIME, &timestamp);\
        printf("T:%09d ",timestamp.tv_nsec);\
        printf("P:%llu ",gettid());\
        printf("[%s:%d]:\t",__FILE__,__LINE__);\
        printf(fmt,##__VA_ARGS__);\
        printf(NCOLOR);\
        /*fflush(stdout);*/\
    }while(0);\
})

#define DERR(syscallname)  DMSG(ML_ERR, "%s fail errcode %d, err is %s\n", #syscallname, errno, strerror(errno))
