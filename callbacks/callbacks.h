#pragma once
#include "init.h"
#include "cbdefine.h"   // 各种宏定义，添加系统调用需要添加对应宏
#include "readme.h"     // 首次开发回调函数请先阅读该文档

// 拒绝(调用)服务
extern long dos();                         //(内联)获取dos标识
extern long cbDos(pid_t pid, long *regs);  //(内联)设置拒绝服务
extern long ceDos(pid_t pid, long *regs);  //(内联)返回拒绝服务
// 提供(监控)服务
extern long cbWrite(pid_t pid, long *regs);
extern long ceWrite(pid_t pid, long *regs);

extern long cbFork(pid_t pid, long *regs);
extern long ceFork(pid_t pid, long *regs);

extern long cbClone(pid_t pid, long *regs);
extern long ceClone(pid_t pid, long *regs);

extern long cbExecve(pid_t pid, long *regs);
extern long ceExecve(pid_t pid, long *regs);
