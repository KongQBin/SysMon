#pragma once
#include "init.h"

// 一个字的长度(__WORDSIZE = 64 or 32 bit)
#define WORDLEN sizeof(long)
// 调用后的情况,此时该系统调用已经执行结束
// 可以做 监听,以及修改其 返回值 等操作
#define IS_END(regs)    (regs->rax + 38)
// 调用前的情况,此时该系统调用还未开始执行
// 可以做 运行控制,以及修改其 实参 等操作
#define IS_BEGIN(regs)  (!IS_END(regs))


#define RET(regs)    regs[g_regsOffset.ret]
#define ARGV_1(regs) regs[g_regsOffset.argv1]
#define ARGV_2(regs) regs[g_regsOffset.argv2]
#define ARGV_3(regs) regs[g_regsOffset.argv3]
#define ARGV_4(regs) regs[g_regsOffset.argv4]
#define ARGV_5(regs) regs[g_regsOffset.argv5]
#define ARGV_6(regs) regs[g_regsOffset.argv6]

extern long writeCallBegin(pid_t pid, long *regs);
extern long writeCallEnd(pid_t pid, long *regs);
