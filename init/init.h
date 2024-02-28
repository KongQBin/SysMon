#pragma once
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/user.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <stdlib.h>
//#include "cbtree.h"
#include "general.h"
#include "testfunc.h"

#ifndef PTRACE_SYSEMU
    #define PTRACE_SYSEMU   31
#endif

#define RET(regs)    regs[gRegsOffset.ret]
#define CALL(regs)   regs[gRegsOffset.call]
#define ARGV_1(regs) regs[gRegsOffset.argv1]
#define ARGV_2(regs) regs[gRegsOffset.argv2]
#define ARGV_3(regs) regs[gRegsOffset.argv3]
#define ARGV_4(regs) regs[gRegsOffset.argv4]
#define ARGV_5(regs) regs[gRegsOffset.argv5]
#define ARGV_6(regs) regs[gRegsOffset.argv6]

typedef struct _RegsOffet
{
    int ret;        //返回值
    int call;       //系统调用号
    int argv1;      //参数一
    int argv2;      //参数二
    int argv3;      //参数三
    int argv4;      //参数四
    int argv5;      //参数五
    int argv6;      //参数六
} RegsOffet;
extern RegsOffet gRegsOffset;
int initRegsOffset();

/* X64 LINUX 6.3.1 REGS
 * struct user_regs_struct
 * r15：通用寄存器，用于存储数据或指针。
 * r14：通用寄存器，用于存储数据或指针。
 * r13：通用寄存器，用于存储数据或指针。
 * r12：通用寄存器，用于存储数据或指针。
 * rbp：基址指针寄存器，指向当前栈帧的基址。
 * rbx：通用寄存器，用于存储数据或指针。
 * r11：通用寄存器，用于存储数据或指针。
 * r10：通用寄存器，用于存储数据或指针。
 * r9： 通用寄存器，用于存储数据或指针。
 * r8： 通用寄存器，用于存储数据或指针。
 * rax：累加器寄存器，用于存储函数返回值。
 * rcx：计数器寄存器，用于存储循环计数值。
 * rdx：数据寄存器，用于存储数据。
 * rsi：源索引寄存器，指向源数据的地址。
 * rdi：目标索引寄存器，指向目标数据的地址。
 * orig_rax：系统调用号。
 * rip：指令指针寄存器，指向下一条要执行的指令。
 * cs：代码段寄存器，存储代码段的段选择符。
 * eflags：标志寄存器，存储程序状态标志。
 * rsp：栈指针寄存器，指向当前栈顶的地址。
 * ss：堆栈段寄存器，存储堆栈段的段选择符。
*/
