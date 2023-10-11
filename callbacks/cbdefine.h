#pragma once
// 一个字的长度(__WORDSIZE = 64 or 32 bit)
#define WORDLEN         sizeof(long)
// 调用后的情况,此时该系统调用已经执行结束
// 可以做 监听,以及修改其 返回值 errno等操作
#define IS_END(regs)    (RET(regs) + 38)
// 调用前的情况,此时该系统调用还未开始执行
// 可以做 运行控制,以及修改其 实参 等操作
#define IS_BEGIN(regs)  (!IS_END(regs))
#define IS_ARCH_64      (sizeof(void*) == sizeof(long long))
/*            此处主要为了解决编译警告            */
#define ID_GETTID       (IS_ARCH_64 ? 186 : 224)
#define gettid()        syscall(ID_GETTID)

/*                    提供服务                    */
/*宏名称（处理器位数？64位系统调用号：32位系统调用号）*/
/*                  文件监控系列                  */
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

/*                  回调函数形参                  */
#define CB_ARGVS_TYPE(av1,av2,av3)    struct pidinfo *av1,long *av2,int av3
#define CB_ARGVS                      CB_ARGVS_TYPE(info,regs,block)

