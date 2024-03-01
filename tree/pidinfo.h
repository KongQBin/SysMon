#pragma once
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "kmstructs.h"
#include "regsoffset.h"
typedef enum _CArgvsType
{
    CAT_NONE = 0,
    CAT_LONG,        // int uint long ulong short float double
    CAT_STRING,      // char *str
    CAT_BYTEARRAY,   // char *byte
} CArgvsType;
typedef struct _CallContext
{
    // 保存寄存器（64猜测够用）
    long regs[64];
    CArgvsType types[REGS_NUMBER];
    // 如果对应的参数是[非数字]类型，那么将其赋值为一个char*指针
    long argvs[REGS_NUMBER];
    // 如果对应的参数是[非数字]类型，则用来存放长度
    long argvsLen[REGS_NUMBER];
} CallContext;

static void clearContextArgvs(CallContext *context)
{
    // 释放可能存在的堆区空间
    for(int i=0; i<REGS_NUMBER; ++i)
    {
        if(context->types[i] != CAT_NONE
            && context->types[i] != CAT_LONG
            && context->argvs[i])
            free((char*)context->argvs[i]);
    }
    // 清空全部
    memset(context,0,sizeof(CallContext));
}

// 被监控的进程信息
typedef struct _PidInfo
{
    struct rb_node node;

    pid_t pid;                  // 进程ID                 （实质是用户层的线程id）
    pid_t gpid;                 // 进程属于哪个进程组     （用户层中的进程id）
    pid_t ppid;                 // 进程的父进程组         （用户层中的父进程id）

    const char *exe;            // 进程的可执行程序路径
    size_t exelen;              // 进程的可执行程序路径的长度

    long long flags;            // setopt、stop状态等
    // 当要对这个进程进行独立管理时
    // 该指针将指向新的内存，否则默认指向gDefaultControlInfo
    const ControlPolicy *cinfo;

    // 是否保留cctext到后置调用
    int reserve;
    // 系统调用参数，放在此处可以使某个系统调用
    // 的前置、后置调用可以共用这个参数
    // 例如：
    //      close的fd参数只在前置调用有效，可以根据fd初始化文件路径
    //      在close的后置调用时，可以根据返回值，来判断操作是否成功
    //      依靠对返回值的判断决定是否向上层发送路径消息
    CallContext cctext;

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
