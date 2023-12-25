#include "callbacks.h"
long cbExecve(CB_ARGVS)
{
    //    printUserRegsStruct(regs);
    size_t len = 0;
    char *str = NULL;
    Interactive *task = argv->task;
    task->argvType[0] = AT_STR;
    if(!getStrArg(argv))
    {
        str = (char*)task->argv[0];
        len = task->argvLen[0];
        if(!getRealPath(argv->info, &str, &len))
            PutMsg(createMsg(ID_EXECVE, argv->block ? BLOCK : NBLOCK,argv->info->gpid,argv->info->pid,str,len,NULL,0));
        else
            DMSG(ML_ERR,"getRegsStrArg err : %s\n",strerror(errno));
    }
    return 0;
}

long ceExecve(CB_ARGVS)
{

    return 0;
}
