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

long cbExecve(CB_ARGVS)
{
    size_t len = 0;
    char *str = NULL;
//    printUserRegsStruct(regs);
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
