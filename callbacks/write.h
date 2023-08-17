#pragma once
#include "cbtree.h"
#include "callbacks.h"

//  写操作前
long writeCallBegin(pid_t pid, long *regs);
//  写操作后
long writeCallEnd(pid_t pid, long *regs);
