#pragma once
#include "init.h"

// 一个字的长度(__WORDSIZE = 64 or 32 bit)
#define WORDLEN sizeof(long)
// 调用后的情况,此时该系统调用已经执行结束
// 可以做 监听,以及修改其 返回值 等操作
#define IS_END(regs)    (regs.rax + 38)        //TODO(kongbin):rax偏移也需要动态获取
// 调用前的情况,此时该系统调用还未开始执行
// 可以做 运行控制,以及修改其 实参 等操作
#define IS_BEGIN(regs)  (!IS_END(regs))

#define IS_ARCH_64      (sizeof(void*) == sizeof(long long))
/*宏名称（处理器位数？64位系统调用号：32位系统调用号）*/
/*                  文件监控系列                  */
#define ID_READ         (IS_ARCH_64 ? 0 : 3)
#define ID_WRITE        (IS_ARCH_64 ? 1 : 4)
#define ID_OPEN         (IS_ARCH_64 ? 2 : 5)   //新版本内核中已经弃用了
#define ID_CLOSE        (IS_ARCH_64 ? 3 : 6)
#define ID_OPENAT       (IS_ARCH_64 ? 257 : 295)
/*                  进程监控系列                  */
#define ID_FORK         (IS_ARCH_64 ? 57 : 2)
#define ID_VFORK        (IS_ARCH_64 ? 58 : 190)
#define ID_EXECVE       (IS_ARCH_64 ? 59 : 11)
#define ID_EXECVEAT     (IS_ARCH_64 ? 322 : 358)
#define ID_KILL         (IS_ARCH_64 ? 62 : 37)

extern long writeCallBegin(pid_t pid, long *regs);
extern long writeCallEnd(pid_t pid, long *regs);
