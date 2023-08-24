#include "init.h"
#include "callbacks.h"

void print_argv(pid_t child, struct user_regs_struct *reg)
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

void print_user_regs_struct(struct user_regs_struct *reg)
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

void mon_sys_call(pid_t child, struct user_regs_struct *reg)
{
//    print_user_regs_struct(reg);
    switch (reg->orig_rax) {
    case __NR_read:

        break;
    case __NR_write:
        IS_BEGIN(reg) ? writeCallBegin(child,(long*)reg) : writeCallEnd(child,(long*)reg);
//        write_call_end(child,reg);

//        print_argv(child,reg);
//        print_user_regs_struct(reg);
        break;
    case __NR_open:

        break;
    case __NR_close:
        break;
    case __NR_openat:
//        print_argv(child,reg);
        break;
    case __NR_kill:
        printf("func is kill !!!!\n");
        //        print_argv(child,reg);
        break;
    default:
        printf("reg->orig_rax is %d\n",reg->orig_rax);
        break;
    }
}

void ptrace_hook(pid_t child) {
    // 被监控的进程id
    printf("called by %d\n", child);
    struct user_regs_struct reg;
    // PTRACE_SYSEMU使得child进程暂停在每次系统调用入口处。
    ptrace(PTRACE_SYSEMU, child, 0, 0);
    int status,signal_number;
    while (1) {
        //        wait(0);

        // 等待子进程受到跟踪因为下一个系统调用而暂停
        waitpid(child,&status,0);
        if (WIFSTOPPED(status)) {
            // Check if the child received a signal
            signal_number = WSTOPSIG(status);
            printf("signal is %d\n",signal_number);
            // kill -15 || kill -2(ctrl + c)
            if(signal_number == 15 || signal_number == 2)
            {
                if (ptrace(PTRACE_CONT, child, NULL, signal_number) == -1) {
                    perror("ptrace continue");
                }
                exit(1);
                continue;
            }
        }

        memset(&reg,0,sizeof(reg));
        // 获取子进程寄存器的值
        ptrace(PTRACE_GETREGS, child, 0, &reg);
        mon_sys_call(child,&reg);
        long ret = ptrace(PTRACE_SYSCALL, child, 0, 0);
        if(ret < 0)
            printf("%s\n",strerror(errno));
    }
}

int main(int argc, char** argv) {
    // 测试动态获取寄存器偏移
//    struct regs_struct_offset offset;
//    init();
//    return 1;
    // 测试map容器
    init();
    return 0;

    pid_t target_pid = atoi(argv[1]);
    // 附加到被传入PID的进程
    if (ptrace(PTRACE_ATTACH, target_pid, 0, 0) == -1) {
        perror("ptrace attach");
        exit(1);
    }

    // 放过发往子进程的信号
    // 但经测，貌似不起作用
//     ptrace(PTRACE_SETOPTIONS, target_pid, NULL, PTRACE_O_TRACESYSGOOD);


    ptrace_hook(target_pid);
    // DETACH注销我们的跟踪,target process恢复运行
    ptrace(PTRACE_DETACH, target_pid, 0, 0);
    return 0;
}
