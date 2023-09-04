#pragma once
#include "init.h"

// 一个字的长度(__WORDSIZE = 64 or 32 bit)
#define WORDLEN sizeof(long)
// 调用后的情况,此时该系统调用已经执行结束
// 可以做 监听,以及修改其 返回值 errno等操作
#define IS_END(regs)    (RET(regs) + 38)
// 调用前的情况,此时该系统调用还未开始执行
// 可以做 运行控制,以及修改其 实参 等操作
#define IS_BEGIN(regs)  (!IS_END(regs))
#define IS_ARCH_64      (sizeof(void*) == sizeof(long long))

/*             拒绝服务(denial of service)                    */
// 不要使用sizeof(long)，因为底层貌似只识别到32位，
// 在long最高位设置为1不影响正常调用（64位系统下）
//#define DOS             (1UL << (WORDLEN*sizeof(long)-1))
#define DOS             (1 << (WORDLEN*sizeof(int)-1))
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

/*
 * PTRACE_POKEDATA：该标志用于将数据写入被跟踪进程的内存。
 * 你可以使用它来修改进程的数据段（data segment）或堆（heap）中的任意内存位置。
 * 例如，你可以使用它来修改程序中的变量值或函数的参数。
 *
 * PTRACE_POKETEXT：该标志用于将指令写入被跟踪进程的内存。
 * 你可以使用它来修改进程的代码段（text segment）中的指令。
 * 通过修改指令，你可以实现一些代码注入或代码替换的操作。
 * 这在进行动态调试或代码修复时可能会有用。
 *
 * PTRACE_POKEUSER：该标志用于修改被跟踪进程的用户寄存器（user registers）。
 * 用户寄存器包括程序计数器（program counter）、堆栈指针（stack pointer）和其他一些与进程状态相关的寄存器。
 * 通过修改这些寄存器的值，你可以对进程的执行流程进行精确控制，例如修改函数调用栈或返回地址等。
 */

/*
 * 注意一：
 *  当使用PTRACE_POKEDATA修改某个系统调用的字符串参数时，
 *  务必保证新字符串长度小于等于原字符串长度，否则会出现以下两种情况
 *      情况1：如果原字符串在数据段，新字符串长度超出，会导致破坏原数据段数据，或其它不可预知的错误
 *          {就比如在我本地while循环中有{write，printf}，这两个系统调用使用的参数均在数据段，
 *        当我修改write使用个的新字符串长度超出原字符串长度的时候，就出现了不调用下一次的系统调用（printf）的问题}
 *      情况2：如果原字符串在堆区，新字符串长度超出，大概率会导致段错误
 * 注意二：
 *  当被修改的数据在数据段
 *      修改会是永久的，只有在下次目标进程重新启动数据才会还原
 *          另外个人猜测如果是动态库的话，则需要使用该动态库的进程全部退出，系统将动态库从内存中释放，
 *      然后重新启动各个进程，将动态库重新加载进内存，数据才会被还原
 *  当被修改的数据在堆区则有两种情况，
 *      情况1：该堆区变量仅被赋值过一次，则现象与修改数据段数据一致
 *      情况2：该堆区变量被赋值过多次，则需要重复修改，才能保证该进程一直使用的是你所修改的数据
 * 注意三：
 *  当被修改的数据是值拷贝传递，那么修改该数据不会影响程序下一次使用该参数
 *  就比如修改write函数中的参数1，也就是fd的值，并不会造成后续调用close导致文件描述符泄漏的问题
 */

// 拒绝服务
extern long cbDos(pid_t pid, long *regs);  //设置拒绝服务
extern long ceDos(pid_t pid, long *regs);  //返回拒绝服务
// 提供服务
extern long cbWrite(pid_t pid, long *regs);
extern long ceWrite(pid_t pid, long *regs);

extern long cbFork(pid_t pid, long *regs);
extern long ceFork(pid_t pid, long *regs);

extern long cbClone(pid_t pid, long *regs);
extern long ceClone(pid_t pid, long *regs);

extern long cbExecve(pid_t pid, long *regs);
extern long ceExecve(pid_t pid, long *regs);
