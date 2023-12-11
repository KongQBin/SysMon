#include "testfunc.h"
//void printArgv(pid_t child, struct user_regs_struct *reg)
//{
//    long temp_long;
//    char message[1000] = {0};
//    char* temp_char2 = message;
//    for(int i=0;i<reg->rdx/WORDLEN+1;++i)
//    {
//        temp_long = ptrace(PTRACE_PEEKDATA, child, reg->rsi + (i*WORDLEN) , NULL);
//        memcpy(temp_char2, &temp_long, WORDLEN);
//        temp_char2 += WORDLEN;
//    }
//    message[reg->rdx] = '\0';
//    dmsg(">>>>>>>>>>    %s\n",message);
//    if(reg->rax != -38 && strstr(message,"123"))
//    {
//        reg->rax = 6;
//        dmsg("mod rax\n");
//    }
//    else if(reg->rax == -38 && strstr(message,"123"))
//    {
//        reg->rdi = 1;
//        dmsg("mod rid\n");
//    }
//    ptrace(PTRACE_SETREGS, child, NULL, reg);
//}

//void printUserRegsStruct(struct user_regs_struct *reg)
//{
//    // 被监控的系统调用的参数
//    //    dmsg("First argument: %llu\n", reg->rdi);
//    //    dmsg("Second argument: %llu\n", reg->rsi);
//    //    dmsg("Third argument: %llu\n", reg->rdx);


//    unsigned long long int* regs = (unsigned long long int*)reg;

//    //    if(regs[3] == reg->r12)         dmsg("AAAAAAAAAAA\n");
//    //    if(regs[4] == reg->rbp)         dmsg("BBBBBBBBBBB\n");
//    //    if(regs[5] == reg->rbx)         dmsg("CCCCCCCCCCC\n");
//    //    if(regs[8] == reg->r9)          dmsg("DDDDDDDDDDD\n");
//    //    if(regs[10] == reg->rax)        dmsg("EEEEEEEEEEE\n");
//    //    if(regs[15] == reg->orig_rax)   dmsg("FFFFFFFFFFF\n");
//    //    if(regs[18] == reg->eflags)     dmsg("GGGGGGGGGGG\n");
//    //    if(regs[20] == reg->ss)         dmsg("HHHHHHHHHHH\n");
//    for(int i=0;i<sizeof(struct user_regs_struct)/sizeof(long);++i)
//        dmsg("%lld\t",regs[i]);
//    dmsg("\n");
//}

void printUserRegsStruct2(struct user *user)
{
    // 被监控的系统调用的参数
    //    dmsg("First argument: %llu\n", reg->rdi);
    //    dmsg("Second argument: %llu\n", reg->rsi);
    //    dmsg("Third argument: %llu\n", reg->rdx);
    long* regs = (long*)user;

    //    if(regs[3] == reg->r12)         dmsg("AAAAAAAAAAA\n");
    //    if(regs[4] == reg->rbp)         dmsg("BBBBBBBBBBB\n");
    //    if(regs[5] == reg->rbx)         dmsg("CCCCCCCCCCC\n");
    //    if(regs[8] == reg->r9)          dmsg("DDDDDDDDDDD\n");
    //    if(regs[10] == reg->rax)        dmsg("EEEEEEEEEEE\n");
    //    if(regs[15] == reg->orig_rax)   dmsg("FFFFFFFFFFF\n");
    //    if(regs[18] == reg->eflags)     dmsg("GGGGGGGGGGG\n");
    //    if(regs[20] == reg->ss)         dmsg("HHHHHHHHHHH\n");
    for(int i=0;i<sizeof(user->regs)/sizeof(long);++i)
        dmsg("%lld\t\t",((long*)&user->regs)[i]);
    dmsg("\n\n");
}
