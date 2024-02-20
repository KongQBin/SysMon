/* 目的：
 * 精简目标文件代码，视觉上更整洁、容易维护
 * 避免真实函数调用过程中压栈出栈等操作，提高性能
 */
//case SIGTRAP:   /*5*/\

/* 0000 0000
 * 1 位 信号来源 如果来自内核 = 1 来自用户 = 0
 * 其余位为信号的值
*/


/*if(WIFSIGNALED(signal))/*kill -9)*/\
/*    {\
            DMSG(ML_INFO,"WIFSIGNALED exit status is %d\n",signal);\
            return AP_TARGET_PROCESS_EXIT;\
    }\
*/
//#define MANAGE_SIGNAL(pid,status,info){\
int signal = (status & 0xFF) ^ (1 << 7);\
dmsg(">>    signal is %d    <<\n",status >> 8);\
switch (signal) {\
case SIGTERM:  /* kill -15 */\
case SIGINT:   /* 2 Ctrl + c */\
case SIGCHLD:  /* 17 子进程的退出或终止事件 */\
    return AP_IS_SIGNAL;\
case SIGTRAP:  /* kill -5 */\
    break;\
}\
}



#define MANAGE_SIGNAL(pid,status,info){\
if(WIFSIGNALED(status))/*由信号导致退出或者正常退出*/\
{\
    DMSG(ML_INFO,"WIFSIGNALED exit signal is %d\n",WTERMSIG(status));\
    GO_END(TT_TARGET_PROCESS_EXIT);\
}\
int tmp = WSTOPSIG(status);\
int signal = (tmp & 0x7F);\
DMSG(ML_INFO,">>pid is %d    signal is %d    origin is %d    event is %d    <<\n",pid,signal,(tmp & 0x80)>>7,status>>16);\
switch (signal) {\
case 0:\
case SIGTRAP:  /* kill -5 */\
case SIGCHLD:  /* 17 子进程的退出或终止事件 */\
    break;\
case SIGSTOP:  /* 19 */\
if(pidSearch(&gDefaultControlInfo->ptree,pid)) \
    break;\
/* 以上信号如果传递下去，会导致监控出现异常，漏监控 */ \
/* 以下信号如果不传递下去，会导致目标进程逻辑异常，当前进程也可能会出现问题 */ \
case SIGSEGV:  /* sig 11 SIGSEGV 启动pycharm会一直触发，如果不传递下去，会导致死循环*/\
case SIGTERM:  /* kill -15 */\
case SIGINT:   /* 2 Ctrl + c */\
case 31:       /*SIGUNUSED?SIGUSR2 用谷歌浏览器打开知乎必定触发该信号，如果不放行就会导致知乎页面卡死*/\
/*default:*/\
    /*DMSG(ML_INFO,"pid is %llu status = %d tmp = %d signal = %d\n",pid,status,tmp,signal);*/\
    status = signal;\
    GO_END(TT_IS_SIGNAL);\
}\
}


// int signal = (status & 0xFF) ^ (1 << 7);\
    dmsg(">>    signal is %d    <<\n",signal);\
    switch (signal) {\
    case SIGTERM:  /* kill -15 */\
    case SIGINT:   /* 2 Ctrl + c */\
        if (ptrace(PTRACE_CONT, pid, NULL, status) < 0)\
        DMSG(ML_WARN,"PTRACE_CONT : %s(%d) pid is %llu\n", strerror(errno),errno,pid);\
        return AP_IS_SIGNAL;\
        break;\
    case SIGCHLD:  /* 17 子进程的退出或终止事件 */\
    if (ptrace(PTRACE_CONT, pid, NULL, status) < 0){\
            DMSG(ML_WARN,"PTRACE_CONT : %s(%d) pid is %llu\n", strerror(errno),errno,pid);\
            return AP_IS_SIGNAL;\
    }\
        break;\
    case SIGTRAP:  /* kill -5 */\
        break;\
}\

#define MANAGE_EVENT(pid,status,info) {\
int event = (status >> 16);\
dmsg(">>>   get event is %d   <<<\n",event);\
switch (event){\
case PTRACE_EVENT_FORK:/*创建进程组*/\
{\
    DMSG(ML_INFO_EVENT,"Event:\tPTRACE_EVENT_FORK target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_FORK,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_VFORK:/*创建虚拟进程*/\
{\
    DMSG(ML_INFO_EVENT,"Event:\tPTRACE_EVENT_VFORK target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_VFORK,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_CLONE:/*创建进程*/\
{\
    DMSG(ML_INFO_EVENT,"Event:\tPTRACE_EVENT_CLONE target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_CLONE,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_EXEC:/*运行可执行程序*/\
{\
    DMSG(ML_INFO_EVENT,"Event:\tPTRACE_EVENT_EXEC target pid is %d\n",pid);\
    getProcId(PTRACE_EVENT_CLONE,pid,status,info);\
    break;\
}\
case PTRACE_EVENT_VFORK_DONE:/*虚拟进程运行结束*/\
{\
    DMSG(ML_INFO_EVENT,"Event:\tPTRACE_EVENT_VFORK_DONE target pid is %d\n",pid);\
    break;\
}\
case PTRACE_EVENT_EXIT:/*进程结束*/\
{\
    DMSG(ML_INFO_EVENT,"Event:\tPTRACE_EVENT_EXIT target pid is %d\n",pid);\
    GO_END(TT_TARGET_PROCESS_EXIT);\
    break;\
}\
case PTRACE_EVENT_STOP:/*进程暂停*/\
{\
    DMSG(ML_INFO_EVENT,"Event:\tPTRACE_EVENT_STOP target pid is %d\n",pid);\
    break;\
}\
default:\
{\
    if(event) DMSG(ML_WARN,"Unknown event is %d!!! Target pid is %d\n",event,pid);\
    break;\
}\
}\
if(event) GO_END(TT_IS_EVENT;)\
}

