#include "callbacks.h"

inline long cbDos(pid_t pid, long *regs)
{
    // 修改系统调用号
    // 起到拒绝服务的目的
    CALL(regs) = CALL(regs) | DOS;
    //    printf("DOS_CLONE = %lld\n",CALL(regs));
    int ret = ptrace(PTRACE_SETREGS, pid, NULL, regs);
    if(ret < 0) perror("cbDos error");
    return 0;
}
inline long ceDos(pid_t pid, long *regs)
{
    // -38 = Function not implemented = 未实现的函数
    // -1  = Operation not permitted  = 不被允许的操作
    // perror或strerror打印错误消息依据errno
    // 由于cbDos中修改了系统调用号为系统中不存在的调用，用户打印errno就会等于-38
    // 为了不漏出马脚（更贴合当前场景），此处修改rax寄存器，该寄存器会影响errno的值
    // 此处我将errno修改为-1，如此就比较符合当前的场景，也就是该系统调用不被我们所允许

    //    printf("rax = %lld\n",RET(regs));
    RET(regs) = -1;
    int ret = ptrace(PTRACE_SETREGS, pid, NULL, regs);
    if(ret < 0)  perror("ceDos error");
    return 0;
}
