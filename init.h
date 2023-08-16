#ifndef INIT_H
#define INIT_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/user.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>

#ifndef PTRACE_SYSEMU
    #define PTRACE_SYSEMU   31
#endif

struct regs_struct_offset
{
    int call;       //系统调用号
    int ret;        //返回值
    int argv1;      //参数一
    int argv2;      //参数二
    int argv3;      //参数三
    int argv4;      //参数四
    int argv5;      //参数五
    int argv6;      //参数六
};

int init(struct regs_struct_offset *offset);
int testMonWrite(struct regs_struct_offset *offset);
#endif // INIT_H
