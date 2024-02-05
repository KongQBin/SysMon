#pragma once
#include "kmstructs.h"
ControlInfo *gDefaultControlInfo;
typedef enum _TASKTYPE
{
    TT_SUCC = 0,                     // 处理成功
    TT_TARGET_PROCESS_EXIT,          // 进程退出
    TT_REGS_READ_ERROR,              // 寄存器读取失败
    TT_CALL_UNREASONABLE,            // 范围外的调用号
    TT_CALL_NOT_FOUND,               // 容器中不存在对该调用的处理
    TT_IS_EVENT,                     // 事件处理
    TT_IS_SIGNAL,                    // 信号处理
    TT_IS_SYSCALL,                   // 系统调用处理
    TT_TO_BLOCK,                     // 阻塞该系统调用
} TASKTYPE;
