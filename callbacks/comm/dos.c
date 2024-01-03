#include "callbacks.h"

/*             拒绝服务(denial of service)                    */
// 不要使用sizeof(long)，因为底层貌似只识别到32位，
// 在long最高位设置为1不影响正常调用（64位系统下）
//#define DOS             (1UL << (WORDLEN*sizeof(long)-1))
#define DOS             (1 << (WORDLEN*sizeof(int)-1))
inline long DoS() { return DOS; }
inline long nDoS(long call) { return ~DOS&call; }

inline long cbDoS(CB_ARGVS)
{
    pid_t *pid = &argv->info->pid;
    long *regs = argv->task->regs;
    // 修改系统调用号为不存在的调用，起到拒绝服务的目的
    CALL(regs) = CALL(regs) | DOS;
    if(ptrace(PTRACE_SETREGS, *pid, NULL, regs) < 0)
        DMSG(ML_WARN,"cbDoS error");
    return 0;
}

inline long ceDoS(CB_ARGVS)
{
    pid_t *pid = &argv->info->pid;
    long *regs = argv->task->regs;
    int block = argv->block;
    // -38 = Function not implemented = 未实现的函数
    // -1  = Operation not permitted  = 不被允许的操作
    // perror或strerror打印错误消息依据errno
    // 由于cbDos中修改了系统调用号为系统中不存在的调用，用户打印errno就会等于-38
    // 为了不漏出马脚（更贴合当前场景），此处修改rax寄存器，该寄存器会影响errno的值
    // 此处我将errno修改为-1，如此就比较符合当前的场景，也就是该系统调用不被我们所允许

    //    printf("rax = %lld\n",RET(regs));
    RET(regs) = -1;
    if(ptrace(PTRACE_SETREGS, *pid, NULL, regs) < 0)
        DMSG(ML_WARN,"cbDoS error");
    return 0;
}
