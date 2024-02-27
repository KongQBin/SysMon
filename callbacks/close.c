#include "callbacks.h"
long cbClose(CB_ARGVS)
{
////    printUserRegsStruct(regs);
    //        DMSG(ML_INFO,"gpid %d, pid %d\n",argv->info->gpid,argv->info->pid);
//    DMSG(ML_INFO,"ARGV_1(argv->cctext.regs) = %d\n",ARGV_1(argv->cctext.regs));
    size_t len = 0;
    int openflag = 0;
    char *path = NULL;
    do{
        // 获取文件描述符的打开标志并判断是否为写权限打开
        if(getFdOpenFlag(argv->info,ARGV_1(argv->cctext.regs),&openflag) || !(O_ACCMODE&openflag))
            break;
        // 获取与描述符对应的文件
        if(!getFdPath(argv->info,ARGV_1(argv->cctext.regs),&path,&len))
            break;
        // 判断回调指针非空
        if(PutMsg)
            PutMsg(createMsg(ID_CLOSE, /*block ? BLOCK : */NBLOCK,argv->info->gpid,
                             argv->info->pid,argv->info->exe,argv->info->exelen,path,len));
    }while(0);
    return 0;
}
long ceClose(CB_ARGVS)
{
//    DMSG(ML_INFO,"close ret = %d\n",RET(argv->cctext.regs));
    if(RET(argv->cctext.regs) == 0)
    {
    }
    return 0;
}
