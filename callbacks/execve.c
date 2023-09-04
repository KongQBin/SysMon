#include "execve.h"

void printUserRegsStruct2(struct user_regs_struct *reg)
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

long cbExecve(pid_t pid, long *regs)
{
    printUserRegsStruct2((struct user_regs_struct *)regs);
    return 0;
}

long ceExecve(pid_t pid, long *regs)
{

    return 0;
}
