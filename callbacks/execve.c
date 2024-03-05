#include "callbacks.h"
/*
 *  execve比较特殊
 *  在框架逻辑中，其寄存器就已经被获取了
 *  在此处只需要简单判断发送即可
*/
long cbExecve(CB_ARGVS)
{
    //    printUserRegsStruct(regs);
    if(argv->cctext->argvsLen[AO_ARGV1] && PutMsg)
        PutMsg(CREATE_MSG(ID_EXECVE,ISBLOCK(argv->cinfo,ID_CLOSE)?BLOCK:NBLOCK,
                          (char*)argv->cctext->argvs[AO_ARGV1],argv->cctext->argvsLen[AO_ARGV1],NULL,0));
    return 0;
}

long ceExecve(CB_ARGVS)
{
    return 0;
}
