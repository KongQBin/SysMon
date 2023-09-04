#pragma once
#include "cbtree.h"
#include "callbacks.h"

//  fork前
long cbFork(pid_t pid, long *regs);
//  fork后
long ceFork(pid_t pid, long *regs);
