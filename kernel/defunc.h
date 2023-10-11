/* 目的：
 * 精简目标文件代码，视觉上更整洁、容易维护
 * 避免真实函数调用过程中压栈出栈等操作，提高性能
 */

#define MANAGE_SIGNAL(pid,status){\
if(WIFSIGNALED(status))/*kill -9)*/\
{\
    DMSG(ML_INFO,"WIFSIGNALED exit signal is %d\n",WTERMSIG(status));\
    return AP_TARGET_PROCESS_EXIT;\
}\
\
int signal = WSTOPSIG(status);\
dmsg(">>    signal is %d    <<\n",signal);\
switch (signal) {\
case SIGTERM:  /*kill -15*/\
case SIGINT:   /*Ctrl + c*/\
/*case SIGCHLD:  /*子进程结束、接收到SIGSTOP停止（挂起）和接收到SIGCONT唤醒时都会向父进程发送SIGCHLD信号*/\
    if (ptrace(PTRACE_CONT, pid, NULL, signal) < 0)\
        DMSG(ML_WARN,"PTRACE_CONT : %s(%d) pid is %llu\n", strerror(errno),errno,pid);\
    return AP_TARGET_PROCESS_EXIT;\
    break;\
default:\
    /*DMSG(ML_WARN,"Unknown signal is %d\n",signal);*/\
    dmsg("Unknown signal is %d\n",signal);\
    break;\
}\
}

#define MANAGE_EVENT(pid,status) {\
int event = (status >> 16);\
dmsg(">>>   get event is %d   <<<\n",event);\
switch (event){\
case PTRACE_EVENT_FORK:/*创建进程组*/\
{\
    dmsg("Event:\tPTRACE_EVENT_FORK target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_FORK,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_VFORK:/*创建虚拟进程*/\
{\
    dmsg("Event:\tPTRACE_EVENT_VFORK target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_VFORK,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_CLONE:/*创建进程*/\
{\
    dmsg("Event:\tPTRACE_EVENT_CLONE target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_CLONE,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_EXEC:/*运行可执行程序*/\
{\
    dmsg("Event:\tPTRACE_EVENT_EXEC target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_CLONE,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_VFORK_DONE:/*虚拟进程运行结束*/\
{\
    dmsg("Event:\tPTRACE_EVENT_VFORK_DONE target pid is %d\n",pid);\
    break;\
}\
case PTRACE_EVENT_EXIT:/*进程结束*/\
{\
    dmsg("Event:\tPTRACE_EVENT_EXIT target pid is %d\n",pid);\
/*    if (ptrace(PTRACE_CONT, pid, NULL, signal) < 0)\
        dmsg("PTRACE_CONT : %s(%d) pid is %d\n", strerror(errno),errno,pid);\*/\
    return AP_TARGET_PROCESS_EXIT;\
}\
case PTRACE_EVENT_STOP:/*进程暂停*/\
{\
    DMSG(ML_WARN,"Event:\tPTRACE_EVENT_STOP target pid is %d\n",pid);\
    break;\
}\
default:\
{\
    if(event) dmsg("Unknown event is %d!!! Target pid is %d\n",event,pid);\
    break;\
}\
}\
if(event)\
{\
    if(ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0)\
    {\
        dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,pid);\
        if(errno == 3) return errno;   /*No such process*/\
    }\
    return AP_IS_EVENT;\
}\
}
