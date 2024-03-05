#pragma once
#include <sys/ptrace.h>
#include <sys/user.h>
#include <errno.h>
#include "general.h"
#include "cbdefine.h"
#include "pidtree.h"
extern int gSeize;
typedef struct
{
    pid_t pid;
    int sig;
    int slient;
} ptraceargs;

static inline int __ptraceAttach(ptraceargs *args)
{
    int traceop = gSeize ? PTRACE_SEIZE : PTRACE_ATTACH;
    long setopt = gSeize ? EVENT_CONCERN : 0L;
    if(ptrace(traceop, args->pid, 0L, setopt) < 0)
    {
        if(!args->slient) DMSG(ML_WARN_2,"PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,args->pid);
        return -1;
    }
    // SEIZE模式不会自动停止，需要手动调用一次PTRACE_INTERRUPT来停止
    // 若不使用PTRACE_INTERRUPT，则后续的系统调用事件不会被抛过来
    if(gSeize && ptrace(PTRACE_INTERRUPT, args->pid, 0L, 0L) < 0)
    {
        if(!args->slient) DMSG(ML_WARN_2,"PTRACE_INTERRUPT : %s(%d) pid is %d\n",strerror(errno),errno,args->pid);
        return -2;
    }
    return 0;
}
static inline int __ptraceDetach(ptraceargs *args)
{
    if(ptrace(PTRACE_DETACH, args->pid, 0, args->sig) < 0)
    {
        if(!args->slient) DMSG(ML_WARN_2,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,args->pid);
        return -1;
    }
    return 0;
}
// 如果默认参数仅仅等于0的话，
// 可以不使用‘_’开头的桥接函数
// 如果默认参数非0，则必须要有以下函数
static inline int _ptraceAttach(ptraceargs *args)
{
    args->sig = args->sig ? args->sig : 0;
    args->slient = args->slient ? args->slient : 0;
    return __ptraceAttach(args);
}
static inline int _ptraceDetach(ptraceargs *args)
{
    args->sig = args->sig ? args->sig : 0;
    args->slient = args->slient ? args->slient : 0;
    return __ptraceDetach(args);
}
// 宏配合gnu结构体赋值形式模拟默认参数函数
#define ptraceAttach(...) _ptraceAttach(&((ptraceargs){__VA_ARGS__}))
#define ptraceDetach(...) _ptraceDetach(&((ptraceargs){__VA_ARGS__}))
