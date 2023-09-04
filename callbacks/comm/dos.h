#pragma once
#include "cbtree.h"
#include "callbacks.h"
long cbDos(pid_t pid, long *regs);  //设置拒绝服务
long ceDos(pid_t pid, long *regs);  //返回拒绝服务
