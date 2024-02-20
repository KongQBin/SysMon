#pragma once
typedef struct
{
    pid_t pid;
    int slient;
} ptraceargs;

static inline int __ptraceAttach(ptraceargs *args)
{
    if(ptrace(PTRACE_ATTACH, args->pid, 0, 0) < 0)
    {
        if(!args->slient) DMSG(ML_WARN_2,"PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,args->pid);
        return -1;
    }
    //    int ret = 0;
    //    do{
    //        if(ptrace(PTRACE_ATTACH, pid, 0, 0) < 0)
    //        {
    //            DMSG(ML_WARN_2,"PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
    //            if(errno == ESRCH)  continue;
    //            if(errno != EPERM)  ret = -1;
    //        }
    //    }while(0);
    return 0;
}
static inline int __ptraceDetach(ptraceargs *args)
{
    if(ptrace(PTRACE_DETACH, args->pid, 0, 0) < 0)
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
    args->slient = args->slient ? args->slient : 0;
    return __ptraceAttach(args);
}
static inline int _ptraceDetach(ptraceargs *args)
{
    args->slient = args->slient ? args->slient : 0;
    return __ptraceDetach(args);
}
#define ptraceAttach(...) _ptraceAttach(&((ptraceargs){__VA_ARGS__}))
#define ptraceDetach(...) _ptraceDetach(&((ptraceargs){__VA_ARGS__}))
