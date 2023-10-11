#pragma once
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "cbdefine.h"

struct pidinfo;
struct ControlInfo
{
    pid_t cpid;                                 // 当前进行监控的线程ID
    pid_t tpid;                                 // 要监控的目标进程ID
    int64_t block[8];                           // 用来判断与bloom对应的系统调用是否需要阻塞
    struct rb_root ptree;                       // 被该线程监控到的进程树
    struct rb_root ftree;                       // 特别关注的文件，目的是在进行特定的文件操作时进行阻塞
    // 用函数指针的形式以空间换时间，既可以调用，又可以作为布隆过滤器
    // long (*func)(pid_t,long *); 函数指针指向的函数
    long (*cbf[512])(CB_ARGVS_TYPE(,,));        // call begin func     在执行系统调用前需要调用的函数   (4kb)
    long (*cef[512])(CB_ARGVS_TYPE(,,));        // call  end  func     在执行系统调用后需要调用的函数   (4kb)
    int toexit;                                 // 退出对tpid的监控
};
#define SETBLOCK(ControlInfo,CALLID)              {ControlInfo->block[CALLID/64] |= 1 << CALLID%64;}
#define UNBLOCK(ControlInfo,CALLID)               {ControlInfo->block[CALLID/64] ^= 1 << CALLID%64;}
#define ISBLOCK(ControlInfo,CALLID)               (ControlInfo->block[CALLID/64] & 1 << CALLID%64)
#define SETFUNC(ControlInfo,CALLID,CBF,CEF)       {ControlInfo->cbf[CALLID] = CBF; ControlInfo->cef[CALLID] = CEF;}
#define UNSETFUNC(ControlInfo,CALLID,CBF,CEF)     SETFUNC(ControlInfo,CALLID,NULL,NULL)
