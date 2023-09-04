#pragma once
#include "cbtree.h"
#include "callbacks.h"
#include "dos.h"

long cbClone(pid_t pid, long *regs);
long ceClone(pid_t pid, long *regs);
