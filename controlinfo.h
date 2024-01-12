#pragma once
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "cbdefine.h"

typedef struct _GlobalData
{
    pid_t mainPid;          // 主线程id
    int threadNumber;       // 线程数量
} GlobalData;

struct pidinfo;
typedef struct _ControlInfo2
{
    pid_t cpid;                                 // 当前进行监控的线程ID
    pid_t tpid;                                 // 要监控的目标进程组ID
    int64_t block[8];                           // 用来判断与bloom对应的系统调用是否需要阻塞
    struct rb_root ptree;                       // 被该线程监控到的进程树
    struct rb_root ftree;                       // 特别关注的文件，目的是在进行特定的文件操作时进行阻塞
    // 用函数指针的形式以空间换时间，既可以调用，又可以作为布隆过滤器
    // long (*func)(pid_t,long *); 函数指针指向的函数
    long (*cbf[512])(CB_ARGVS_TYPE());        // call begin func     在执行系统调用前需要调用的函数   (4kb)
    long (*cef[512])(CB_ARGVS_TYPE());        // call  end  func     在执行系统调用后需要调用的函数   (4kb)
    int toexit;                                 // 退出监控
    int exit;                                   // 已退出

    pthread_cond_t  *cond;  // 追踪线程的条件变量
    void *taddr;            // 工作线程给追踪线程的结构指针
} ControlInfo2;


