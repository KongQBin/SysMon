#include "callbacks.h"
long cbExecve(CB_ARGVS)
{
    //    printUserRegsStruct(regs);
    size_t len = 0;
    char *str = NULL;
    if(!getArg(&argv->info->pid,&ARGV_1(argv->cctext->regs),(void*)&str,&len))
    {
        if(!getRealPath(argv->info, &str, &len))
        {
            PutMsg(createMsg(ID_EXECVE, ISBLOCK(argv->cinfo,ID_CLOSE)?BLOCK:NBLOCK,argv->info->gpid,argv->info->pid,
                             argv->info->exe, argv->info->exelen,str,len,NULL,0));

            // 保存变量（勿删）
            argv->cctext->types[AO_ARGV1] = CAT_STRING;
            argv->cctext->argvsLen[AO_ARGV1] = len;
            argv->cctext->argvs[AO_ARGV1] = (long)str;
        }
        else
            DMSG(ML_ERR,"getRegsStrArg err : %s\n",strerror(errno));
    }
    return 0;
}

long ceExecve(CB_ARGVS)
{
    return 0;
}
