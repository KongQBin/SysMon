#include "procmain.h"

extern int globalexit;
const char *WhiteList[] = {
    "Xorg",                 // 图形界面绘制，调用巨量的写(write)函数
    "SysMon",
    "JYNZDFY",
    "JYNGLTX",
    "JYNGJCZ",
    "filemonitor",
    "ZyUpdate",
    "ZyUDiskTray",
};

static int initControlInfoCallBackInfo()
{
    // 设置监控到关注的系统调用，在其执行前后的调用函数
    //    SETFUNC(gDefaultControlPolicy,ID_WRITE,cbWrite,ceWrite);
    //    SETFUNC(gDefaultControlPolicy,ID_FORK,cbFork,ceFork);
    //    SETFUNC(gDefaultControlPolicy,ID_CLONE,cbClone,ceClone);
    SETFUNC(gDefaultControlPolicy,ID_EXECVE,cbExecve,ceExecve);
    SETFUNC(gDefaultControlPolicy,ID_CLOSE,cbClose,ceClose);
    SETFUNC(gDefaultControlPolicy,ID_RENAME,cbRename,ceRename);
    SETFUNC(gDefaultControlPolicy,ID_RENAMEAT,cbRenameat,ceRenameat);
    SETFUNC(gDefaultControlPolicy,ID_RENAMEAT2,cbRenameat2,ceRenameat2);
    //    SETFUNC(gDefaultControlPolicy,ID_OPENAT,cbOpenat,ceOpenat);
    gPidTree.rb_node = NULL;
    gDefaultControlPolicy->ftree.rb_node = NULL;
    gDefaultControlPolicy->dtree.rb_node = NULL;
    // 设置阻塞模式
    //    SETBLOCK(info,ID_EXECVE);
    return 0;
}

static int ifNotContinue()
{
    int toBreak = 0;
    DMSG(ML_WARN,"Wait pid return -1. %s(%d).\n", strerror(errno), errno);
    switch (errno)
    {
    case ECHILD:    // 没有被追踪的进程了，退出循环
        toBreak = 1;
    case EINTR:     // 单纯被信号打断,接着wait
        break;
    case EINVAL:    // 无效参数
    default:        // 其它错误
        toBreak = 1;
        break;
    }
    return toBreak;
}

static void getProcId(int evtType,pid_t pid,int status,ControlPolicy *info)
{
    return;
    pid_t spid;
    // 获取新进程的PID
    if(ptrace(PTRACE_GETEVENTMSG, pid, NULL, &spid) >= 0)
    {
//        dmsg("Child process created: %d status = %d\n", spid, status);
        if(spid <= 0 ) return;
        PidInfo *pinfo = pidSearch(&gPidTree,pid);
        if(!pinfo)
        {
//            dmsg("Unknown parent process\n");
            return;
        }
        switch (evtType) {
        case PTRACE_EVENT_FORK: // 进程组
            pidInsert(&gPidTree,createPidInfo(spid,spid,pinfo->gpid));
            break;
        case PTRACE_EVENT_VFORK: // 虚拟进程
            pidInsert(&gPidTree,createPidInfo(spid,spid,pinfo->gpid));
            break;
        case PTRACE_EVENT_CLONE: // 进程
            pidInsert(&gPidTree,createPidInfo(spid,pinfo->gpid,pinfo->ppid));
            break;
        default:
            break;
        }
    }
    else
        dmsg("PTRACE_GETEVENTMSG : %s(%d) pid is %d\n", strerror(errno),errno,spid);
}

// 检查是否在白名单中
int checkWhite(PidInfo *pinfo)
{
    // 初始化校验标识，证明被检查过了
    SET_CHKWHITE(pinfo->flags);
    // 路径为空或获取失败
    if(!pinfo->exe && getExe(pinfo,(char**)&pinfo->exe,&pinfo->exelen) == -1)
        return 1;
    for(int i=0;i<sizeof(WhiteList)/sizeof(WhiteList[0]);++i)
        if(strstr(pinfo->exe,WhiteList[i]))
        {
            DMSG(ML_WARN,"%s in WhiteList\n",pinfo->exe);
            return 1;
        }
    return 0;
}

#define GO_END(type) {tasktype = type; goto END;}
static CbArgvs av;
void onProcessTask(pid_t *pid, int *status)
{
    TASKTYPE tasktype = TT_SUCC;
    PidInfo *pinfo = pidSearch(&gPidTree,*pid);
    if(!pinfo)
    {
        if(pinfo = createPidInfo(*pid,0,0))
            pidInsert(&gPidTree,pinfo);
        else
            DMSG(ML_ERR,"%llu createPidInfo fail\n",*pid);
    }
    if(!pinfo->cinfo) pinfo->cinfo = gDefaultControlPolicy;
    gCurrentControlPolicy = pinfo->cinfo;

    // 证明未查询到且创建/初始化失败 或 退出flag为真
    if(!pinfo || globalexit)
        GO_END(TT_TARGET_PROCESS_EXIT);
    // 判断是否校验过静态白名单，如果没有则进行校验，校验为白，则取消对其的追踪
    if(!CHKWHITED(pinfo->flags) && checkWhite(pinfo))
        GO_END(TT_TARGET_PROCESS_EXIT);
    // ATTACH模式下 设置options
    if(!gSeize && !IS_SETOPT(pinfo->flags))
    {
        if(ptrace(PTRACE_SETOPTIONS, pinfo->pid, NULL, EVENT_CONCERN) < 0)
            DMSG(ML_WARN,"PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),pinfo->pid);
        else
            SET_SETOPT(pinfo->flags);
    }

    pinfo->status = *status;
    // 分析是信号还是事件
    sigEvt(pinfo,&tasktype);
    // 如果不是系统调用，那么就跳转到END
    if(tasktype != TT_IS_SYSCALL) GO_END(tasktype);


    /*      如果是系统调用，那么进行以下处理流程     */
    // 初始化寄存器容器
    struct user user;
    long *regs = (long*)&user.regs;

    // 获取寄存器
    if(ptrace(PTRACE_GETREGS, *pid, 0, regs) < 0)
    {
        DMSG(ML_ERR,"PTRACE_GETREGS: %s(%d) %llu\n", strerror(errno),errno,*pid);
        GO_END(TT_REGS_READ_ERROR);
    }

    int callid = nDoS(CALL(regs));
    if(callid < 0 || callid >= CALL_MAX)      // 判断系统调用号在一个合理范围
        GO_END(TT_CALL_UNREASONABLE);
//    if(callid == ID_EXIT_GROUP)             // 进程退出
//        GO_END(TT_TARGET_PROCESS_EXIT);

    // 指针数组作为bloom使用，判断是否关注该系统调用
    if(IS_BEGIN(regs) ? !gCurrentControlPolicy->cbf[callid] : !gCurrentControlPolicy->cef[callid])
        GO_END(TT_CALL_NOT_FOUND);

//    DMSG(ML_INFO,"From *pid %d\tHit Call %d %s\n",*pid,callid,IS_BEGIN(regs)?"Begin":"End");
    if(!pinfo)
    {
        if(pinfo = createPidInfo(*pid,0,0))
            pidInsert(&gPidTree,pinfo);
    }
    if(pinfo)
    {
        // 判断新的调用号与老的调用号不匹配
        int oldcallod = nDoS(CALL(pinfo->cctext.regs));
        if(oldcallod && callid != oldcallod)
            // 清空老的系统调用上下文信息
            clearContextArgvs(&pinfo->cctext);

        // 初始化业务处理回调参数
        av.info = pinfo;
        av.cinfo = pinfo->cinfo;
        av.cctext = &pinfo->cctext;
        av.clearContext = &pinfo->clearCctext;
        memcpy(pinfo->cctext.regs,regs,sizeof(user.regs));
        // 调用业务处理回调函数
        IS_BEGIN(regs) ?
            gCurrentControlPolicy->cbf[callid](&av):
            gCurrentControlPolicy->cef[callid](&av);
        // 强行更新可执行程序路径及长度
        if(callid == ID_EXECVE && !IS_BEGIN(regs)
            && pinfo->cctext.argvsLen[AO_ARGV1]
            && RET(pinfo->cctext.regs) == 0)
        {
            // 释放原始空间
            free((char*)pinfo->exe);
            // 强行赋值
            char **exe = (char**)&pinfo->exe;
            *exe = (char*)pinfo->cctext.argvs[AO_ARGV1];
            pinfo->exelen = pinfo->cctext.argvsLen[AO_ARGV1];
            // 避免被释放
            pinfo->cctext.types[AO_ARGV1] = CAT_NONE;
            pinfo->cctext.argvsLen[AO_ARGV1] = 0;
            pinfo->cctext.argvs[AO_ARGV1] = 0;
        }

        if(*av.clearContext || !IS_BEGIN(regs))
            // 清空老的系统调用上下文信息
            clearContextArgvs(&pinfo->cctext);
    }
    else
    {
        /*
         * pinfo = NULL 这种情况出现的场景可能是：
         * 一：
         * 上述代码既没有查询到info，又在创建info时失败了
         * 二：
         * 目标进程组A被监控前，新的可执行程序B已经被启动
         * 在这个进程B在退出时会通知进程组A，也会出现查询不到的情况
         * 无需关心该问题，进程组B已经被其它监控线程监控了
         */
        DMSG(ML_WARN,"Current *pid %d is not in pid tree.\n",*pid);
    }
    tasktype = TT_SUCC;
END:
//    DMSG(ML_WARN,"Task type = %d\n",tasktype);

    switch (tasktype)
    {
    case TT_CALL_NOT_FOUND:
    case TT_SUCC:
    case TT_IS_EVENT:       //事件直接放行
    case TT_TO_BLOCK:
    case TT_REGS_READ_ERROR:
    case TT_CALL_UNREASONABLE:
//         放行该任务(也可能是一个事件)
        if(ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0)
            DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
        break;
    case TT_IS_SIGNAL:    //放行信号
        // 继续该任务（信号）
        if(ptrace(PTRACE_SYSCALL, *pid, 0, pinfo->status) < 0)
            DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) *pid is %d\n",strerror(errno),errno,*pid);
        break;
    case TT_IS_SIGNAL_STOP:     /*仅在SEIZE模式生效*/
        // 进入暂停且监听的状态（模拟对STOP信号的响应）
        if(ptrace(PTRACE_LISTEN, *pid, 0, 0) < 0)
            DMSG(ML_WARN,"PTRACE_LISTEN : %s(%d) *pid is %d\n",strerror(errno),errno,*pid);
        break;
    case TT_TARGET_PROCESS_EXIT:
        /*
         * 进程退出
         * 两种退出形式，一种是正常退出 系统调用号(callid) = ID_EXIT_GROUP
         * 另一种是由于信号导致 ctrl + c || kill -9 || kill -15
         */
        // 取消对该进程的追踪，进入下一个循环
        // DMSG(ML_INFO,"*pid : %d to exit!\n",*pid);
        if(ptrace(PTRACE_DETACH, *pid, 0, 0) < 0)
            DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
        pidDelete(&gPidTree,*pid);
        break;
    default:
        DMSG(ML_WARN,"Unknown TASKTYPE = %d\n",tasktype);
        break;
    }
}

extern int globalexit;
static void sigOptions(int sig)
{
    if(sig == SIGINT || sig == SIGTERM)
    {
        ++ globalexit;  // 1 = 软退出 2 = 强制退出
        if(globalexit >= 2)
        {
            ManageInfo info;
            info.type = MT_ToExit;
            taskOpt(&info,NULL);
        }
    }
}
void MonProcMain(pid_t cpid)
{
    signal(SIGINT,sigOptions);  // Ctrl + c
    signal(SIGTERM,sigOptions); // kill -15
    do{
        gDefaultControlPolicy = calloc(1,sizeof(ControlPolicy));
        if(!gDefaultControlPolicy)
        {
            DERR(calloc);
            break;
        }
        // 初始化监控信息
        initControlInfoCallBackInfo();
        // 开始监控'控制线程'
        if(ptraceAttach(cpid))
        {
            DMSG(ML_ERR,"PTRACE_ATTACH : %s(%d) pid is %d\n",
                 strerror(errno),errno,cpid);
            break;
        }
        pidInsert(&gPidTree,createPidInfo(cpid,0,0));

        // 设置回调
        setTaskOptFunc(taskOpt);

        pid_t npid;
        int status;
        while(1)
        {
            status = 0;
            npid = wait4(-1,&status,WUNTRACED|__WALL,0);
            if(npid == -1 && ifNotContinue())   break;                              // 判断是否应该进入下个循环
            if(npid == cpid)                    onControlThreadMsg(cpid,status);    // 这一般是来自主进程的控制信息
            else                                onProcessTask(&npid,&status);       // 响应被监控进程反馈的事件
        }
    }while(0);
    DMSG(ML_INFO,"MonProcMain to return\n");
    return ;
}
