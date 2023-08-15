#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <string.h>
#include <errno.h>

/*
 * struct user_regs_struct
 *
 * r15：通用寄存器，用于存储数据或指针。
 * r14：通用寄存器，用于存储数据或指针。
 * r13：通用寄存器，用于存储数据或指针。
 * r12：通用寄存器，用于存储数据或指针。
 * rbp：基址指针寄存器，指向当前栈帧的基址。
 * rbx：通用寄存器，用于存储数据或指针。
 * r11：通用寄存器，用于存储数据或指针。
 * r10：通用寄存器，用于存储数据或指针。
 * r9：通用寄存器，用于存储数据或指针。
 * r8：通用寄存器，用于存储数据或指针。
 * rax：累加器寄存器，用于存储函数返回值。
 * rcx：计数器寄存器，用于存储循环计数值。
 * rdx：数据寄存器，用于存储数据。
 * rsi：源索引寄存器，指向源数据的地址。
 * rdi：目标索引寄存器，指向目标数据的地址。
 * orig_rax：系统调用号。
 * rip：指令指针寄存器，指向下一条要执行的指令。
 * cs：代码段寄存器，存储代码段的段选择符。
 * eflags：标志寄存器，存储程序状态标志。
 * rsp：栈指针寄存器，指向当前栈顶的地址。
 * ss：堆栈段寄存器，存储堆栈段的段选择符。
*/

// 一个字的长度(__WORDSIZE = 64 or 32 bit)
#define WORDLEN sizeof(long)
// 调用后的情况,此时该系统调用已经执行结束
// 可以做 监听,以及修改其 返回值 等操作
#define IS_END(regs)    (regs->rax + 38)
// 调用前的情况,此时该系统调用还未开始执行
// 可以做 运行控制,以及修改其 实参 等操作
#define IS_BEGIN(regs)  (!IS_END(regs))

void print_argv(pid_t child, struct user_regs_struct *reg);
void write_call_begin(pid_t child, struct user_regs_struct *reg)
{
    long temp_long;
    char message[1000] = {0};
    char* temp_char2 = message;
    for(int i=0;i<reg->rdx/WORDLEN+1;++i)
    {
        temp_long = ptrace(PTRACE_PEEKDATA, child, reg->rsi + (i*WORDLEN), NULL);
        memcpy(temp_char2, &temp_long, WORDLEN);
        temp_char2 += WORDLEN;
    }
    message[reg->rdx] = '\0';

    char *test = "abcdefghijklmn";
    char *ctmp = test;
    if(!strcmp(message,"12345") || strstr(message,"abcde"))
    {
        printf("rdx = %d\n",reg->rdx);
        printf("message = %s\n",message);
        printf("A: ");
        for(int i=0;i<strlen(message)+1;++i)
        {
            printf("0x%x\t",((char*)&message)[i]);
        }
        printf("\n");

        // 修改write的参数1为1
        // 也就是将文件描述符修改为标准输出
        reg->rdi = 1;
        reg->rdx = strlen(test)+1;
        printf("S: reg->rdi = %d\n",reg->rdi);
        printf("S: reg->rdx = %d\n",reg->rdx);
        long ret = ptrace(PTRACE_SETREGS, child, NULL, reg);
        if(ret < 0)
            printf("%s\n",strerror(errno));
        if(strstr(message,"abcde")) return;

        long tmp;
        for(int i=0; i<strlen(test)/WORDLEN + 1; ++i)
        {
            printf("B: ");
            tmp = 0;
            ctmp += (i*WORDLEN);
            memcpy(&tmp,ctmp,strlen(ctmp) < WORDLEN ? strlen(ctmp) : WORDLEN);
            for(int j=0;j<WORDLEN;++j)
            {
                printf("0x%x\t",((char*)&tmp)[j]);
            }
            printf("\n");
            long ret = ptrace(PTRACE_POKEDATA, child, reg->rsi + (i*WORDLEN), tmp);
            printf("ptrace ret: %d\n",ret);
            if(ret < 0) printf("%s\n",strerror(errno));
        }


//        {
//            long temp_long;
//            char message[1000] = {0};
//            char* temp_char2 = message;
//            for(int i=0;i<reg->rdx/WORDLEN+1;++i)
//            {
//                temp_long = ptrace(PTRACE_PEEKDATA, child, reg->rsi + (i*WORDLEN), NULL);
//                memcpy(temp_char2, &temp_long, WORDLEN);
//                temp_char2 += WORDLEN;
//            }
//            message[reg->rdx] = '\0';

//            printf("C: ");
//            for(int j = 0; j<sizeof(temp_long); ++j)
//            {
//                printf("0x%x ",((char*)&temp_long)[j]);
//            }
//            printf("\nmessage2 = %s\n",message);
//        }
    }
}
void write_call_end(pid_t child, struct user_regs_struct *reg)
{
    long temp_long;
    char message[1000] = {0};
    char* temp_char2 = message;
    for(int i=0;i<reg->rdx/WORDLEN+1;++i)
    {
        temp_long = 0;
        temp_long = ptrace(PTRACE_PEEKDATA, child, reg->rsi + (i*WORDLEN) , NULL);
        memcpy(temp_char2, &temp_long, WORDLEN);
        temp_char2 += WORDLEN;
    }
    if(strstr(message,"abcdefg"))
    {
        printf("E: reg->rdx = %d\n",reg->rdx);
        //    message[reg->rdx] = '\0';
        printf("EE: ");
        for(int i=0;i<WORDLEN;++i)
            printf("0x%x\t",message[i]);
        printf("\nwrite(%d,\"%s\")\n",reg->rdi,message);
        printf("\n\n\n");

        //    // 修改返回值(可以根据已获取到的条件)
        //    reg->rax = 6;
        //    ptrace(PTRACE_SETREGS, child, NULL, reg);
    }
}

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

void mon_sys_calls_x64(pid_t child, struct user_regs_struct *reg)
{
//    print_user_regs_struct(reg);
    switch (reg->orig_rax) {
    case __NR_read:

        break;
    case __NR_write:
        IS_BEGIN(reg) ? write_call_begin(child,reg) : write_call_end(child,reg);
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
        wait(0);
        memset(&reg,0,sizeof(reg));
        // 获取子进程寄存器的值
        ptrace(PTRACE_GETREGS, child, 0, &reg);
        mon_sys_calls_x64(child,&reg);
        long ret = ptrace(PTRACE_SYSCALL, child, 0, 0);
        if(ret < 0)
            printf("%s\n",strerror(errno));
    }
}

int main(int argc, char** argv) {
//    SysMon();
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
