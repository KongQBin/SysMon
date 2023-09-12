#include "callbacks.h"

long cbClone(pid_t pid, long *regs)
{
//    cbDos(pid,regs);
    return 0;
}
int createMonThread(pid_t *pid);

long ceClone(pid_t pid, long *regs)
{
//    ceDos(pid,regs);
//    if(CALL(regs) & dos())
//    {
//        return 0;
//    }
//    else
//    {

//    }

//    if(RET(regs) > 0)
//    {
//        // 再拉起一个监控线程去进行监控
//        createMonThread(RET(regs));
//    }
//    else if(!RET(regs))
//    {
//        usleep(100);
//    }
    return 0;
}
