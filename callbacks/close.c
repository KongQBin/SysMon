#include "callbacks.h"
// 针对close的监控，主要为上层文件监控业务提供支持
long cbClose(CB_ARGVS)
{
//    printUserRegsStruct(regs);
//    DMSG(ML_INFO,"gpid %d, pid %d\n",argv->info->gpid,argv->info->pid);
//    DMSG(ML_INFO,"ARGV_1(argv->cctext.regs) = %d\n",ARGV_1(argv->cctext.regs));

    size_t len = 0;
    int openflag = 0;
    char *path = NULL;
    do{
        // 获取文件描述符的打开标志并判断是否为写权限打开
        if(getFdOpenFlag(argv->info,ARGV_1(argv->cctext->regs),&openflag) || !(O_ACCMODE&openflag))
            break;
        // 获取与描述符对应的文件
        if(!getFdPath(argv->info,ARGV_1(argv->cctext->regs),&path,&len))
            break;
        // 保存变量
        SAVE_ARGV(AO_ARGV1,CAT_STRING,(long)path,len);
    }while(0);
    return 0;
}
long ceClose(CB_ARGVS)
{
//    DMSG(ML_INFO,"close ret = %d\n",RET(argv->cctext->regs));
//    DMSG(ML_INFO,"argv->cctext->argvs[AO_ARGV1] = %lu,argv->cctext->argvsLen[AO_ARGV1] = %d\n",
//         argv->cctext->argvs[AO_ARGV1],argv->cctext->argvsLen[AO_ARGV1]);

    // 这边判断一下长度，因为cbClose中的openflag、fdpath判断失败会退出
    // 此处如果不加以判断，则会将一些空信息传递到上层，浪费性能
    if(argv->cctext->argvsLen[AO_ARGV1] && RET(argv->cctext->regs) == 0 && PutMsg)
        PutMsg(CREATE_MSG(ID_CLOSE,NBLOCK,(char*)argv->cctext->argvs[AO_ARGV1],argv->cctext->argvsLen[AO_ARGV1],NULL,0));
    return 0;
}
