#pragma once
#include <unistd.h>
#include <pthread.h>
// 一个字的长度(__WORDSIZE = 64 or 32 bit)
#define WORDLEN         sizeof(long)
// 调用后的情况,此时该系统调用已经执行结束
// 可以做 监听,以及修改其 返回值 errno等操作
#define IS_END(regs)    (RET(regs) + 38)
// 调用前的情况,此时该系统调用还未开始执行
// 可以做 运行控制,以及修改其 实参 等操作
#define IS_BEGIN(regs)  (!IS_END(regs))
//#define IS_ARCH_64      (sizeof(void*) == sizeof(long long))
#define IS_ARCH_64      (__WORDSIZE == 64)
/*            此处主要为了解决编译警告            */
#define ID_GETTID       (IS_ARCH_64 ? 186 : 224)
#define gettid()        syscall(ID_GETTID)
#define tkill(tid,sig)  syscall(__NR_tkill,tid,sig)
//#define ID_GETDENTS64   (IS_ARCH_64 ? 217 : 220)
//#define getdents64(fd,buf,size) syscall(ID_GETDENTS64,fd,buf,size)

/*                                   ptrace 宏                                  */
/*部分处理器架构下的系统没有这些宏，但大多系统这些宏的值是一致的，故此处进行定义*/
#ifndef PT_SETREGS
    #define PT_SETREGS 13
#endif
#ifndef PT_GETREGS
    #define PT_GETREGS 12
#endif

//#ifndef PTRACE_EVENT_FORK
//    #define PTRACE_EVENT_FORK       1
//#endif
//#ifndef PTRACE_EVENT_VFORK
//    #define PTRACE_EVENT_VFORK      2
//#endif
//#ifndef PTRACE_EVENT_CLONE
//    #define PTRACE_EVENT_CLONE      3
//#endif
//#ifndef PTRACE_EVENT_EXEC
//    #define PTRACE_EVENT_EXEC       4
//#endif
//#ifndef PTRACE_EVENT_VFORK_DONE
//    #define PTRACE_EVENT_VFORK_DONE 5
//#endif
//#ifndef PTRACE_EVENT_EXIT
//    #define PTRACE_EVENT_EXIT       6
//#endif
//#ifndef PTRACE_EVENT_SECCOMP
//    #define PTRACE_EVENT_SECCOMP    7
//#endif
//#ifndef PTRACE_EVENT_STOP
//    #define PTRACE_EVENT_STOP       128
//#endif

/*                    提供服务                    */
/*宏名称（处理器位数？64位系统调用号：32位系统调用号）*/
/*                  文件监控系列                  */
#define READ_DIFF_ARCH

#define ID_READ         (IS_ARCH_64 ? 0 : 3)
#define ID_WRITE        (IS_ARCH_64 ? 1 : 4)
#define ID_OPEN         (IS_ARCH_64 ? 2 : 5)    //从kernel 2.26开始 glibc将open重定向到了openat，且新版本内核中已经不存在该调用了
#define ID_CLOSE        (IS_ARCH_64 ? 3 : 6)
#define ID_OPENAT       (IS_ARCH_64 ? 257 : 295)
/*                  进程监控系列                  */
#define ID_CLONE        (IS_ARCH_64 ? 56 : 120)
#define ID_FORK         (IS_ARCH_64 ? 57 : 2)   //从kernel 2.3.3开始 fork被clone替换
#define ID_VFORK        (IS_ARCH_64 ? 58 : 190)
#define ID_EXECVE       (IS_ARCH_64 ? 59 : 11)
#define ID_EXECVEAT     (IS_ARCH_64 ? 322 : 358)
#define ID_KILL         (IS_ARCH_64 ? 62 : 37)
#define ID_EXIT_GROUP   (IS_ARCH_64 ? 231 : 252)

#define EVENT_CONCERN \
(PTRACE_O_TRACESYSGOOD|PTRACE_O_TRACEEXEC|\
 PTRACE_O_TRACEEXIT|PTRACE_O_TRACECLONE|\
 PTRACE_O_TRACEFORK|PTRACE_O_TRACEVFORK)
