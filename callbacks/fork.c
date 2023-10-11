#include "callbacks.h"

long cbFork(CB_ARGVS)
{
    return 0;
//    CALL(regs) = 10000;
//    int ret = ptrace(PTRACE_SETREGS, pid, NULL, regs);
//    if(ret < 0) printf("%s\n",strerror(errno));
//    return 0;
}

long ceFork(CB_ARGVS)
{
    return 0;
}
