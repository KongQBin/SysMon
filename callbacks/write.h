#pragma once
#include "cbtree.h"
#include "callbacks.h"

//  写操作前
long cbWrite(pid_t pid, long *regs);
//  写操作后
long ceWrite(pid_t pid, long *regs);
