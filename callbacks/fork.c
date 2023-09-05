#include "callbacks.h"

long cbFork(pid_t pid, long *regs)
{
    CALL(regs) = 10000;
    int ret = ptrace(PTRACE_SETREGS, pid, NULL, regs);
    if(ret < 0) printf("%s\n",strerror(errno));
    return 0;
}

long ceFork(pid_t pid, long *regs)
{
    if(RET(regs) > 0)
    {
        // 再拉起一个监控线程去进行监控
    }
    return 0;
}
