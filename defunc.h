/* 目的：
 * 精简目标文件代码，视觉上更整洁、容易维护
 * 避免真实函数调用过程中压栈出栈等操作，提高性能
 */

#define MANAGE_EVENT(event) {\
switch (event){\
case PTRACE_EVENT_FORK:/*创建进程*/\
{\
    dmsg("Event:\tPTRACE_EVENT_FORK target pid is %d\n",*pid);\
    printProcId(pid,status);\
    break;\
}\
case PTRACE_EVENT_VFORK:/*创建虚拟进程*/\
{\
    dmsg("Event:\tPTRACE_EVENT_VFORK target pid is %d\n",*pid);\
    printProcId(pid,status);\
    break;\
}\
case PTRACE_EVENT_CLONE:/*？？创建线程*/\
{\
    dmsg("Event:\tPTRACE_EVENT_CLONE target pid is %d\n",*pid);\
    printProcId(pid,status);\
    break;\
}\
case PTRACE_EVENT_EXEC:/*运行可执行程序*/\
{\
    dmsg("Event:\tPTRACE_EVENT_EXEC target pid is %d\n",*pid);\
    break;\
}\
case PTRACE_EVENT_VFORK_DONE:/*？？虚拟进程结束*/\
{\
    dmsg("Event:\tPTRACE_EVENT_VFORK_DONE target pid is %d\n",*pid);\
    break;\
}\
case PTRACE_EVENT_EXIT:/*进程结束*/\
{\
    dmsg("Event:\tPTRACE_EVENT_EXIT target pid is %d\n",*pid);\
    if (ptrace(PTRACE_CONT, *pid, NULL, sig) < 0)\
        dmsg("PTRACE_CONT : %s(%d) pid is %d\n", strerror(errno),errno,*pid);\
    return A_EXIT;\
}\
case PTRACE_EVENT_STOP:/*进程暂停*/\
{\
    dmsg("Event:\tPTRACE_EVENT_STOP target pid is %d\n",*pid);\
    break;\
}\
default:\
{\
    if(event) dmsg("Unknown event!!! target pid is %d\n",*pid);\
    break;\
}\
}\
}
