#include "init.h"
#include "callbacks.h"

void printArgv(pid_t child, struct user_regs_struct *reg)
{
    long temp_long;
    char message[1000] = {0};
    char* temp_char2 = message;
    for(int i=0;i<reg->rdx/WORDLEN+1;++i)
    {
        temp_long = ptrace(PTRACE_PEEKDATA, child, reg->rsi + (i*WORDLEN) , NULL);
        memcpy(temp_char2, &temp_long, WORDLEN);
        temp_char2 += WORDLEN;
    }
    message[reg->rdx] = '\0';
    printf(">>>>>>>>>>    %s\n",message);
    if(reg->rax != -38 && strstr(message,"123"))
    {
        reg->rax = 6;
        printf("mod rax\n");
    }
    else if(reg->rax == -38 && strstr(message,"123"))
    {
        reg->rdi = 1;
        printf("mod rid\n");
    }
     ptrace(PTRACE_SETREGS, child, NULL, reg);
}

void printUserRegsStruct(struct user_regs_struct *reg)
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

void monSysCall(pid_t child)
{
     struct user_regs_struct reg;
     memset(&reg,0,sizeof(reg));
     // 获取子进程寄存器的值
     ptrace(PTRACE_GETREGS, child, 0, &reg);

     long *pregs = (long*)&reg;
     //    printUserRegsStruct(&reg);
     struct syscall *call = cbSearch(CALL(pregs));
     if(!call)
     {
//         printf("CALL(pregs):%d doesn't exist in callback tree!\n",CALL(pregs));
         return;
     }
     printf("Call %d\n",CALL(pregs));
     IS_BEGIN(reg) ? ((long (*)(pid_t,long *))call->cBegin)(child,pregs) : ((long (*)(pid_t,long *))call->cEnd)(child,pregs);
}

void ptraceHook(pid_t child) {
    // 被监控的进程id
    printf("called by %d\n", child);
    // PTRACE_SYSEMU使得child进程暂停在每次系统调用入口处。
    ptrace(PTRACE_SYSEMU, child, 0, 0);
    int status,signal_number;
    while (1)
    {
//         wait(0);
        // 等待子进程受到跟踪因为下一个系统调用而暂停
        waitpid(child,&status,0);
        if (WIFSTOPPED(status))
        {
            // Check if the child received a signal
            signal_number = WSTOPSIG(status);
//            printf("signal is %d\n",signal_number);
            // kill -15 || kill -2(ctrl + c)
            if(signal_number == 15 || signal_number == 2)
            {
                if (ptrace(PTRACE_CONT, child, NULL, signal_number) == -1) {
                    printf("PTRACE_CONT : %s(%d)\n",strerror(errno),errno);
                }
                continue;
            }
        }
        monSysCall(child);
        long ret = ptrace(PTRACE_SYSCALL, child, 0, 0);
        if(ret < 0)
        {
            printf("PTRACE_SYSCALL : %s(%d)\n",strerror(errno),errno);
            if(errno == 3) break;   //No such process
        }
    }
}

int main(int argc, char** argv)
{
    init();
    pid_t target_pid = atoi(argv[1]);
    // 附加到被传入PID的进程
    if (ptrace(PTRACE_ATTACH, target_pid, 0, 0) == -1) {
        printf("PTRACE_ATTACH : %s(%d)\n",strerror(errno),errno);
        exit(1);
    }
    ptraceHook(target_pid);
    // DETACH注销我们的跟踪,target process恢复运行
    ptrace(PTRACE_DETACH, target_pid, 0, 0);
    unInit();
    return 0;
}

// 放过发往子进程的信号
// 但经测，貌似不起作用
//     ptrace(PTRACE_SETOPTIONS, target_pid, NULL, PTRACE_O_TRACESYSGOOD);
