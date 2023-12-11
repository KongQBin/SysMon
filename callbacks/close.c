#include "callbacks.h"
long cbClose(CB_ARGVS)
{
//    printUserRegsStruct(regs);
    char *path = NULL;
    size_t len = 0;
    int openflag = 0;
    if(!getFdOpenFlag(info,ARGV_1(regs),&openflag) && O_ACCMODE&openflag    /*获取openflag成功且非只读打开*/
        && !getFdPath(info,ARGV_1(regs),&path,&len) && PutMsg)              /*获取文件路径成功且PutMsg回调非空*/
        PutMsg(createMsg(ID_CLOSE, block ? BLOCK : NBLOCK,info->gpid,info->pid,NULL,0,path,len));
    return 0;
}
long ceClose(CB_ARGVS)
{

    return 0;
}
