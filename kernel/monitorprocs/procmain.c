#include "procmain.h"

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

extern int globalexit;
static int initControlInfoCallBackInfo()
{
    // 设置监控到关注的系统调用，在其执行前后的调用函数
    SETFUNC(gDefaultControlInfo,ID_WRITE,cbWrite,ceWrite);
    //    SETFUNC(info,ID_FORK,cbFork,ceFork);
    //    SETFUNC(info,ID_CLONE,cbClone,ceClone);
    //    SETFUNC(info,ID_EXECVE,cbExecve,ceExecve);
    //    SETFUNC(info,ID_CLOSE,cbClose,ceClose);
    //    SETFUNC(info,ID_OPENAT,cbOpenat,ceOpenat);
    gDefaultControlInfo->ptree.rb_node = NULL;
    gDefaultControlInfo->ftree.rb_node = NULL;
    gDefaultControlInfo->dtree.rb_node = NULL;
    // 设置阻塞模式
    //    SETBLOCK(info,ID_EXECVE);
    return 0;
}

static int ifNotContinue()
{
    int ret = 0;
    DMSG(ML_ERR,"pid = -1 errno = %d err = %s\n", errno, strerror(errno));
    switch (errno)
    {
    case ECHILD:    // 没有被追踪的进程了，退出循环
        ret = 1;
    case EINTR:     // 单纯被信号打断,接着wait
        break;
    case EINVAL:    // 无效参数
    default:        // 其它错误
        ret = 1;
//        dmsg("%s\n",strerror(errno));
        break;
    }
    return ret;
}

static void getProcId(int evtType,pid_t pid,int status,ControlInfo *info)
{
    return;
    pid_t spid;
    // 获取新进程的PID
    if(ptrace(PTRACE_GETEVENTMSG, pid, NULL, &spid) >= 0)
    {
//        dmsg("Child process created: %d status = %d\n", spid, status);
        if(spid <= 0 ) return;
        PidInfo *pinfo = pidSearch(&info->ptree,pid);
        if(!pinfo)
        {
//            dmsg("Unknown parent process\n");
            return;
        }
        switch (evtType) {
        case PTRACE_EVENT_FORK: // 进程组
            pidInsert(&info->ptree,createPidInfo(spid,spid,pinfo->gpid));
            break;
        case PTRACE_EVENT_VFORK: // 虚拟进程
            pidInsert(&info->ptree,createPidInfo(spid,spid,pinfo->gpid));
            break;
        case PTRACE_EVENT_CLONE: // 进程
            pidInsert(&info->ptree,createPidInfo(spid,pinfo->gpid,pinfo->ppid));
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
    if(!pinfo->exe && getExe(pinfo,&pinfo->exe,&pinfo->exelen) == -1)
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
#define TRAP_SIG (SIGTRAP|0x80)
static CbArgvs av;
void onProcessTask(pid_t *pid, int *status)
{
    TASKTYPE tasktype = TT_SUCC;
    PidInfo *pinfo = pidSearch(&gDefaultControlInfo->ptree,*pid);
    if(!pinfo)
    {
        if(pinfo = createPidInfo(*pid,0,0))
            pidInsert(&gDefaultControlInfo->ptree,pinfo);
        else
            DMSG(ML_ERR,"%llu createPidInfo fail\n",*pid);
    }

    // 证明未查询到且创建失败
    // 为了不干扰目标进程正常运行，取消对它的追踪
    if(!pinfo || globalexit)
        GO_END(TT_TARGET_PROCESS_EXIT);
    if(!CHKWHITED(pinfo->flags) && checkWhite(pinfo))
        GO_END(TT_TARGET_PROCESS_EXIT);
    pinfo->status = *status;
    // 分析是信号还是事件
    sigEvt(pinfo,&tasktype);
    // 如果不是系统调用，那么就跳转到END
    if(tasktype != TT_IS_SYSCALL) GO_END(tasktype);
    // 如果是系统调用，那么进行以下处理流程

    //    MANAGE_SIGNAL(*pid,*status,gDefaultControlInfo);   /*信号处理*/
    //    MANAGE_EVENT(*pid,*status,gDefaultControlInfo);    /*事件处理*/

    struct user user;
    long *regs = (long*)&user.regs;
    // 获取寄存器
    if(ptrace(PTRACE_GETREGS, *pid, 0, regs) < 0)
    {
        DMSG(ML_ERR,"PTRACE_GETREGS: %s(%d) %llu\n", strerror(errno),errno,*pid);
        GO_END(TT_REGS_READ_ERROR);
    }

    int callid = nDoS(CALL(regs));
    if(callid < 0 || callid >= CALL_MAX)    // 判断系统调用号在一个合理范围
        GO_END(TT_CALL_UNREASONABLE);
//    if(callid == ID_EXIT_GROUP)             // 进程退出
//        GO_END(TT_TARGET_PROCESS_EXIT);

    // 指针数组作为bloom使用，判断是否关注该系统调用
    if(IS_BEGIN(regs) ? !gDefaultControlInfo->cbf[callid] : !gDefaultControlInfo->cef[callid])
        GO_END(TT_CALL_NOT_FOUND);

//    DMSG(ML_INFO,"From *pid %d\tHit Call %d\n",*pid,callid);
    if(!pinfo)
    {
        if(pinfo = createPidInfo(*pid,0,0))
            pidInsert(&gDefaultControlInfo->ptree,pinfo);
    }
    if(pinfo)
    {
        // 判断是否已经setoptions了
        if(!IS_SETOPT(pinfo->flags))
        {
//            DMSG(ML_WARN,"Current *pid %d PTRACE_SETOPTIONS\n",*pid);
            if(ptrace(PTRACE_SETOPTIONS, *pid, NULL, EVENT_CONCERN) < 0)
            {
                DMSG(ML_WARN,"PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),*pid);
            }
            else
                SET_SETOPT(pinfo->flags);
        }

        memset(&av,0,sizeof(CbArgvs));
//        av.block = ISBLOCK(info,callid);
        av.info = pinfo;
        av.cinfo = gDefaultControlInfo;
        av.cctext.regs = regs;
//        av.task = task;
//        av.td = td;
        IS_BEGIN(regs) ? gDefaultControlInfo->cbf[callid](&av): gDefaultControlInfo->cef[callid](&av);
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
        DMSG(ML_WARN,"Current *pid %d is not in ptree\n",*pid);
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
//        // 设置下次监控的类型
//        if(ptrace(PTRACE_SETOPTIONS, *pid, NULL, EVENT_CONCERN) < 0)
//            DMSG(ML_WARN,"PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),*pid);
//         放行该任务(也可能是一个事件)
        if(ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0)
            DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
        break;
    case TT_IS_SIGNAL:    //部分信号直接放行
        // 继续该任务（信号）
        if(ptrace(PTRACE_CONT, *pid, 0, pinfo->status) < 0)
            DMSG(ML_WARN,"PTRACE_CONT : %s(%d) *pid is %d\n",strerror(errno),errno,*pid);
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
        pidDelete(&gDefaultControlInfo->ptree,*pid);
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
        gDefaultControlInfo = calloc(1,sizeof(ControlInfo));
        if(!gDefaultControlInfo)
        {
            DMSG(ML_ERR,"calloc fail errcode is %d, err is %s\n",errno,strerror(errno));
            break;
        }
        // 初始化监控信息
        initControlInfoCallBackInfo();
        // 开始监控
        if(!ptraceAttach(cpid))
            pidInsert(&gDefaultControlInfo->ptree,createPidInfo(cpid,0,0));
        // 设置回调
        setTaskOptFunc(taskOpt);

        pid_t npid;
        int status;
        while(1)
        {
            //
            status = 0;
            npid = wait4(-1,&status,/*WNOHANG|WUNTRACED*/__WALL,0);
            if(npid == -1 && ifNotContinue())   break;                              // 判断是否应该进入下个循环
            if(npid == cpid)                    onControlThreadMsg(cpid,status);    // 这一般是来自主进程的控制信息
            else                                onProcessTask(&npid,&status);       // 响应被监控进程反馈的事件
        }
    }while(0);
    DMSG(ML_INFO,"MonProcMain to return\n")
    return ;
}
