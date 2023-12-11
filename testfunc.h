#pragma once
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/user.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include "general.h"
#include "cbdefine.h"

///*做平台适配时使用的函数*/
//void printArgv(pid_t child, struct user_regs_struct *reg);
//void printUserRegsStruct(struct user_regs_struct *reg);
void printUserRegsStruct2(struct user *user);
