#pragma once
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "general.h"

#define ONE_PROC_INFO_SIZE 10                   // 单次增删ProcInfo的大小
#define ONE_PTHREAD_SIZE ONE_PROC_INFO_SIZE     // 单次增删pthread_t的大小
struct ProcInfo
{
    pid_t pid;              // 进程ID
    pthread_t *tid;         // 进程所创建出的线程
    unsigned long tidLen;
    unsigned long tidSize;
};
struct ThreadInfo           // 当前进程信息
{
    pthread_t tid;

    pid_t *pids;                    // 监控的进程信息
    struct ProcInfo **pInfo;       // 监控的进程信息
    unsigned long pidSize;          // 当前空间长度
    unsigned long pidLen;           // 当前存储有效pid长度
    unsigned long pInfoLen;
    unsigned long pInfoSize;
};

int initThreadInfo(struct ThreadInfo *info);
int addPid(struct ThreadInfo *pInfo, pid_t *pid);
int delPid(struct ThreadInfo *pInfo, pid_t *pid);
