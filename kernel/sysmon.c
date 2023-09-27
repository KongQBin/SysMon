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
//        addPid(pInfo,&pid);
        dmsg("Child process created: %d status = %d\n", spid, status);
    }
    else
        dmsg("PTRACE_GETEVENTMSG : %s(%d) pid is %d\n", strerror(errno),errno,spid);
}
void getProcId(struct ThreadInfo *pInfo,pid_t pid,int status);

struct rb_root *cbTree = NULL;
pid_t contpid = 0;
pthread_t thread_id;

struct threadArgs
{
    pid_t pid;
    struct rb_root *cbTree;
};

enum ANALYSISRET
{
    A_SUCC = 0,                     //处理成功
    A_TARGET_PROCESS_EXIT = 1,      //进程退出
    A_REGS_READ_ERROR = 2,          //寄存器读取失败
    A_CALL_NOT_FOUND = 3,           //容器中不存在对该调用的处理
};

enum ANALYSISRET analysis(pid_t *pid,int *status,struct ThreadInfo *pInfo,struct ControlInfo *info, int *callid)
{
    dmsg(" waitpid is %d\n",*pid);
    dmsg(">     status is %d     <\n",*status);
    /*注意这两个宏函数存在return的情况*/
    MANAGE_SIGNAL(*pid,*status);
    MANAGE_EVENT(*pid,*status);

    // 设置下次监控的类型
    if(ptrace(PTRACE_SETOPTIONS, *pid, NULL, EVENT_CONCERN))
        dmsg("PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),*pid);

    struct user_regs_struct reg;
    memset(&reg,0,sizeof(reg));
    // 获取子进程寄存器的值
    if(ptrace(PTRACE_GETREGS, *pid, 0, &reg) < 0)
    {
        dmsg("PTRACE_GETREGS: %s(%d)\n", strerror(errno),*pid);
        return A_REGS_READ_ERROR;
    }

    // printUserRegsStruct(&reg);
    long *pregs = (long*)&reg;

    // 指针数组作为bloom使用，判断是否关注该系统调用
    if(!info->cbf[ndos(CALL(pregs))])
        return A_CALL_NOT_FOUND;
    // 目标进程退出
    if(CALL(pregs) == ID_EXIT_GROUP)
    {
        dmsg("Call is ID_EXIT_GROUP\n");
        return A_TARGET_PROCESS_EXIT;
    }

    dmsg("Hit Call %d\n",CALL(pregs));
    *callid = ndos(CALL(pregs));
    IS_BEGIN(pregs) ?
        info->cbf[*callid](pid,pregs,ISBLOCK(info,*callid)):
        info->cef[*callid](pid,pregs,ISBLOCK(info,*callid));
    return A_SUCC;
}

int controls(pid_t *pid,int *status,int64_t *block)
{
    // 非阻塞
    if(!*block)
    {
//        if (ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0) {
//            dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
//            return -1;
//        }
    }
    //阻塞模式
    else
    {
        // 检查 容器 遍历已经处理完成的事件 然后放行
        // PTRACE_SYSEMU使得pid进程暂停在每次系统调用入口处。
        //    if (ptrace(PTRACE_SYSEMU, mtargs->pid, 0, 0) < 0) {
        //        dmsg("PTRACE_SYSEMU : %s(%d) pid is %d\n",strerror(errno),errno,mtargs->pid);
        //        //        return NULL;
        //    }
    }
    return 0;
}

//if(ptrace(PTRACE_SYSCALL, contpid, 0, 0) < 0)
//    dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,contpid);

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

void* startMon(void* pinfo)
{
    struct ControlInfo *info = (struct ControlInfo *)pinfo;
    info->cpid = gettid();

    struct ThreadInfo *pInfo = NULL;
//    pInfo = calloc(1,sizeof(struct ThreadInfo));
//    if(!pInfo)          goto END;
//    pInfo->tid = gettid();
//    pInfo->pidSize = 10;
//    pInfo->pids = (pid_t*)calloc(pInfo->pidSize,sizeof(pid_t));      //提前准备10个堆区空间
//    if(!pInfo->pids)    goto END;
//    pInfo->pidLen = 0;

    registerSignal();

    pid_t pid = 0;
    int status,ret;
    int toControls;
    int callid = 0;
    dmsg("startMon pid is %d\n",info->tpid);
    // 附加到被传入PID的进程
    if(ret = ptrace(PTRACE_ATTACH, info->tpid, 0, 0))
    {
        dmsg("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,info->tpid);
        goto END;
    }

//    if(!addPid(pInfo,&nfo->tpid))
//        printf("StartMon pthread_t = %llu\n",pInfo->tid);
//    else
//        goto END;

    int run = 1;
    while(run)
    {
        callid = 0;
        status = 0;
        toControls = 1;
        pid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0); //非阻塞 -> WNOHANG

        // 判断pid
        if(pid == -1)
        {
            switch (errno) {
            case ECHILD:    // 没有被追踪的进程了
                run = 0;
            case EINTR:     // 被信号打断
                continue;
                break;
            case EINVAL:    // 无效参数
            default:        // 其它错误
                run = 0;
                dmsg("%s\n",strerror(errno));
                continue;
                break;
            }
        }
        // 开始处理
        if(pid && status)
        {
            enum ANALYSISRET ret = analysis(&pid,&status,pInfo,info,&callid);        // 分析
            switch (ret) {
            case A_SUCC:
                toControls = 1;
                break;
            case A_TARGET_PROCESS_EXIT:
                /* 进程（包括子进程）或者线程退出
                 *
                 * 两种退出形式，一种是正常退出 系统调用号(callid) = ID_EXIT_GROUP
                 * 另一种是由于信号导致 ctrl + c || kill -9 || kill -15
                 */
                // delPid(pInfo,&pid);
                toControls = 0;
                printf("pid : %d to exit!\n",pid);
                // 取消对该进程的追踪，进入下一个循环
                if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
                    dmsg("PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
                continue;
                break;
            case A_CALL_NOT_FOUND:
                dmsg("Syscall:%d doesn't exist in callback bloom!\n",callid);
                break;
            default:
                dmsg("Unknown ANALYSISRET = %d\n",ret);
                break;
            }
            if(toControls) controls(&pid,&status,&info->block[callid]);        // 处理
        }

        // 继续该事件
        if(ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) {
            dmsg("%s : %s(%d) pid is %d\n", "PTRACE_SYSCALL", strerror(errno),errno,pid);
        }
    }

    // unInit(targs->cbTree);

END:
    if(pInfo && pInfo->pids) {free(pInfo->pids);pInfo->pids=NULL;}
    if(pInfo)       {free(pInfo);pInfo=NULL;}
    info->tpid = 0;
    info->toexit = 1;
    return NULL;
}
