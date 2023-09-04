#include "callbacks.h"

long cbClone(pid_t pid, long *regs)
{
    cbDos(pid,regs);
    return 0;
}

long ceClone(pid_t pid, long *regs)
{
    if(CALL(regs) & DOS)
    {
        ceDos(pid,regs);
        return 0;
    }
    else
    {

    }
    return 0;
}
