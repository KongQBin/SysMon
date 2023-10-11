#include "callbacks.h"

long cbClone(struct pidinfo *info, long *regs)
{
//    cbDoS(pid,regs);
    return 0;
}

long ceClone(struct pidinfo *info, long *regs)
{
//    ceDoS(pid,regs);
//    if(CALL(regs) & DoS())
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
