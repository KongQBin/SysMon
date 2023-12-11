#include "callbacks.h"
long cbExecve(CB_ARGVS)
{
    //    printUserRegsStruct(regs);
    size_t len = 0;
    char *str = NULL;
    if(!getRegsStrArg(info, ARGV_1(regs),&str,&len) && !getRealPath(info, &str, &len))
        PutMsg(createMsg(ID_EXECVE, block ? BLOCK : NBLOCK,info->gpid,info->pid,str,len,NULL,0));
    else
        DMSG(ML_ERR,"getRegsStrArg err : %s\n",strerror(errno));
    return 0;
}

long ceExecve(CB_ARGVS)
{

    return 0;
}
