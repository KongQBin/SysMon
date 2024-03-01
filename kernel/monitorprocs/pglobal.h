#pragma once
#include <signal.h>
#include "kmstructs.h"
#define TRAP_SIG (SIGTRAP|0x80)       // 由于我们的追踪导致触发的信号

extern int gSeize;                     // SEIZE模式与ATTACH模式
extern int gProcNum;                   // 用于进行系统监控的进程总数
extern int gPipeToMain[2];             // 用于给主进程进行通讯的管道
extern int gPipeFromMain[2];           // 用于从主进程获取信息的管道
struct rb_root gPidTree;               // 所监控的进程
extern InitInfo gInitInfo[PROC_MAX];   // 用于保存最初的初始化信息
extern ControlPolicy *gDefaultControlPolicy;
extern ControlPolicy *gCurrentControlPolicy;
typedef enum _TASKTYPE
{
    TT_SUCC = 0,                     // 处理成功
    TT_TARGET_PROCESS_EXIT,          // 进程退出
    TT_REGS_READ_ERROR,              // 寄存器读取失败
    TT_CALL_UNREASONABLE,            // 范围外的调用号
    TT_CALL_NOT_FOUND,               // 容器中不存在对该调用的处理
    TT_IS_EVENT,                     // 事件处理
    TT_IS_SIGNAL,                    // 信号处理
    TT_IS_SIGNAL_STOP,               // STOP信号
    TT_IS_SYSCALL,                   // 系统调用处理
    TT_TO_BLOCK,                     // 阻塞该系统调用
} TASKTYPE;
