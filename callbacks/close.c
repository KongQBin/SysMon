#include "callbacks.h"

static void printUserRegsStruct(struct user_regs_struct *reg)
{
    // 被监控的系统调用的参数
    //    printf("First argument: %llu\n", reg->rdi);
    //    printf("Second argument: %llu\n", reg->rsi);
    //    printf("Third argument: %llu\n", reg->rdx);


    unsigned long long int* regs = (unsigned long long int*)reg;

    //    if(regs[3] == reg->r12)         printf("AAAAAAAAAAA\n");
    //    if(regs[4] == reg->rbp)         printf("BBBBBBBBBBB\n");
    //    if(regs[5] == reg->rbx)         printf("CCCCCCCCCCC\n");
    //    if(regs[8] == reg->r9)          printf("DDDDDDDDDDD\n");
    //    if(regs[10] == reg->rax)        printf("EEEEEEEEEEE\n");
    //    if(regs[15] == reg->orig_rax)   printf("FFFFFFFFFFF\n");
    //    if(regs[18] == reg->eflags)     printf("GGGGGGGGGGG\n");
    //    if(regs[20] == reg->ss)         printf("HHHHHHHHHHH\n");
    for(int i=0;i<sizeof(struct user_regs_struct)/sizeof(unsigned long long int);++i)
        printf("%lld\t",regs[i]);
    printf("\n");
}

long cbClose(struct pidinfo *info, long *regs)
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
long ceClose(struct pidinfo *info, long *regs)
{

    return 0;
}
