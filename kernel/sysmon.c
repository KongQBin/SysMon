#include "sysmon.h"

#define EVENT_CONCERN \
(PTRACE_O_TRACESYSGOOD|PTRACE_O_TRACEEXEC|\
PTRACE_O_TRACEEXIT|PTRACE_O_TRACECLONE|\
PTRACE_O_TRACEFORK|PTRACE_O_TRACEVFORK)

inline void getProcId(struct ThreadInfo *pInfo,pid_t pid,int status)
{
    pid_t spid;
    //            // 获取子进程的PID
    if(ptrace(PTRACE_GETEVENTMSG, pid, NULL, &spid) >= 0)
    {
        addPid(pInfo,&pid);
        dmsg("Child process created: %d status = %d\n", spid, status);
    }
    else
        dmsg("PTRACE_GETEVENTMSG : %s(%d) pid is %d\n", strerror(errno),errno,spid);
}
void getProcId(struct ThreadInfo *pInfo,pid_t pid,int status);

struct rb_root *cbTree = NULL;
pid_t contpid = 0;
pthread_t thread_id;

int monSysCall(struct rb_root *cbTree,pid_t child)
{
    struct user_regs_struct reg;
    memset(&reg,0,sizeof(reg));
    // 获取子进程寄存器的值
    ptrace(PTRACE_GETREGS, child, 0, &reg);

    long *pregs = (long*)&reg;
    //    printUserRegsStruct(&reg);
    //     dmsg("Call %d\n",CALL(pregs));
    // 目标进程退出
    if(CALL(pregs) == ID_EXIT_GROUP) return -1;

    struct syscall *call = searchCallbackTree(cbTree,CALL(pregs));
    if(!call)
    {
//        printf("CALL(pregs):%d doesn't exist in callback tree!\n",CALL(pregs));
        return -2;
    }

    dmsg("Call %d\n",CALL(pregs));
    IS_BEGIN(pregs) && !(CALL(pregs)&dos()) ?
        ((long (*)(pid_t,long *))call->cbf)(child,pregs) :
        ((long (*)(pid_t,long *))call->cef)(child,pregs);
    return CALL(pregs);
}

struct threadArgs
{
    pid_t pid;
    struct rb_root *cbTree;
};

enum ANALYSISRET
{
    A_SUCC = 0,
    A_EXIT = 1,
};

enum ANALYSISRET analysis(pid_t *pid,int *status,struct ThreadInfo *pInfo)
{
//    dmsg(" waitpid is %d\n",targs->pid);
    dmsg("> status = %d\n",*status);
    /*注意这两个宏函数存在return的情况*/
    MANAGE_SIGNAL(*pid,*status);
    MANAGE_EVENT(*pid,*status);

    printf("thread_id = %d\n",*status>>16);
    printf("process_id = %d\n",*status & 0xFFFF);
    // 设置下次监控的类型
    if(ptrace(PTRACE_SETOPTIONS, *pid, NULL, EVENT_CONCERN))
        dmsg("PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),*pid);

    int monRet = monSysCall(cbTree,*pid);
    if(monRet != -2) printf("monRet = %d\n",monRet);
    if(monRet == -1) printf("stopMon pid is %d\n", *pid);
    if(monRet != -3)
    {
        if(ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0)
        {
            dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
            if(errno == 3) return errno;   //No such process
        }
    }
    else
    {
        contpid = *pid;
    }
    return 0;
}

int controls(pid_t *pid,int *status)
{
    // 检查 容器 遍历已经处理完成的事件 然后放行


    // PTRACE_SYSEMU使得pid进程暂停在每次系统调用入口处。
    //    if (ptrace(PTRACE_SYSEMU, mtargs->pid, 0, 0) < 0) {
    //        dmsg("PTRACE_SYSEMU : %s(%d) pid is %d\n",strerror(errno),errno,mtargs->pid);
    //        //        return NULL;
    //    }
    return 0;
}

void signalHandler(int signum) {
    if (signum == SIGUSR1) {
        printf("pthread_t = %llu get signal is SIGUSR1 cont pid is %d\n",gettid(),contpid);
        if(ptrace(PTRACE_SYSCALL, contpid, 0, 0) < 0)
            dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,contpid);
    }
}

void registerSignal()
{
    // 注册信号处理函数
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
}

void* startMon(void* ppid)
{
    struct ThreadInfo *pInfo = NULL;
    pInfo = calloc(1,sizeof(struct ThreadInfo));
    if(!pInfo)          goto END;
    pInfo->tid = gettid();
    pInfo->pidSize = 10;
    pInfo->pids = (pid_t*)calloc(pInfo->pidSize,sizeof(pid_t));      //提前准备10个堆区空间
    if(!pInfo->pids)    goto END;
    pInfo->pidLen = 0;

    registerSignal();


    pid_t pid = *(pid_t*)ppid;
    free(ppid); ppid = NULL;
    int status,ret;
    int toControls;
    dmsg("startMon pid is %d\n",pid);
    // 附加到被传入PID的进程
    if(ret = ptrace(PTRACE_ATTACH, pid, 0, 0))
    {
        dmsg("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
        goto END;
    }

    if(!addPid(pInfo,&pid))
        printf("StartMon pthread_t = %llu\n",pInfo->tid);
    else
        goto END;

    while (1)
    {
        status = 0;
        toControls = 1;
        pid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0);
        if(pid && status)
        {
            enum ANALYSISRET ret = analysis(&pid,&status,pInfo);        // 分析
            switch (ret) {
            case A_SUCC:
                toControls = 1;
                break;
            case A_EXIT:
                delPid(pInfo,&pid);
                toControls = 0;
                break;
            default:
                dmsg("Unknown ANALYSISRET = %d\n",ret);
                break;
            }
            if(toControls) controls(&pid,&status);        // 处理
        }
    }

    // DETACH注销我们的跟踪,target process恢复运行
    ptrace(PTRACE_DETACH, pid, 0, 0);
    //    unInit(targs->cbTree);

END:
    if(ppid)        {free(ppid);ppid=NULL;}
    if(pInfo->pids) {free(pInfo->pids);pInfo->pids=NULL;}
    if(pInfo)       {free(pInfo);pInfo=NULL;}
    return NULL;
}
