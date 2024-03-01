#pragma once
#include "kmstructs.h"
// 被监控的进程信息
typedef struct _PidInfo
{
    struct rb_node node;

    pid_t pid;                  // 进程ID                 （实质是用户层的线程id）
    pid_t gpid;                 // 进程属于哪个进程组     （用户层中的进程id）
    pid_t ppid;                 // 进程的父进程组         （用户层中的父进程id）

    char *exe;                  // 进程的可执行程序路径
    size_t exelen;              // 进程的可执行程序路径的长度

    long long flags;                    // setopt、stop状态等
    // 当要对这个进程进行独立管理时
    // 该指针将指向新的内存，否则默认指向gDefaultControlInfo
    ControlPolicy *cinfo;

    int status;         // 当前进程返回的状态
} PidInfo;
#define PID_STOP        0
#define PID_SETOPT      1
#define PID_CHKWHITE    2   // 是否进行过静态白名单校验

#define SET_STOP(flags)         (flags |= 1<<PID_STOP)
#define SET_SETOPT(flags)       (flags |= 1<<PID_SETOPT)
#define SET_CHKWHITE(flags)     (flags |= 1<<PID_CHKWHITE)
#define UNSET_STOP(flags)       (flags &= ~(1<<PID_STOP))
#define UNSET_SETOPT(flags)     (flags &= ~(1<<PID_SETOPT))
#define IS_STOP(flags)          (flags & (1<<PID_STOP))
#define IS_SETOPT(flags)        (flags & (1<<PID_SETOPT))
#define CHKWHITED(flags)        (flags & (1<<PID_CHKWHITE))
