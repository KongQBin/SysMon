#include "workprocess.h"
static ControlInfo *info;
static int ptraceAttach(pid_t pid)
{
    int ret = 0;
    do
    {
        if(ptrace(PTRACE_ATTACH, pid, 0, 0) < 0)
        {
            DMSG(ML_WARN_2,"PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            if(errno == 3)  continue;
            if(errno != 1)  ret = -1;
        }
    }while(0);
    return ret;
}

// 用来处理来自‘控制线程’的任务
static void taskOpt(ManageInfo *minfo,ControlBaseInfo *cbinfo)
{
    do{
        if(!info)   break;
        switch (minfo->type) {
        case MT_Init:
            memcpy(&info->binfo,cbinfo,sizeof(ControlBaseInfo));
            break;
        case MT_AddTid:
            DMSG(ML_INFO,"MT_AddTid : %llu\n",minfo->tpid);
            if(!ptraceAttach(minfo->tpid))
                pidInsert(&info->ptree,createPidInfo(minfo->tpid,0,0));
            break;
        default:
            break;
        }
    }while(0);
}

static int initControlInfoCallBackInfo()
{
    // 设置监控到关注的系统调用，在其执行前后的调用函数
    SETFUNC(info,ID_WRITE,cbWrite,ceWrite);
    //    SETFUNC(info,ID_FORK,cbFork,ceFork);
    //    SETFUNC(info,ID_CLONE,cbClone,ceClone);
    //    SETFUNC(info,ID_EXECVE,cbExecve,ceExecve);
    //    SETFUNC(info,ID_CLOSE,cbClose,ceClose);
    //    SETFUNC(info,ID_OPENAT,cbOpenat,ceOpenat);
    info->ptree.rb_node = NULL;
    info->ftree.rb_node = NULL;
    info->dtree.rb_node = NULL;
    // 设置阻塞模式
    //    SETBLOCK(info,ID_EXECVE);
    return 0;
}

static int ifContinue()
{
    int ret = 0;
    DMSG(ML_ERR,"pid = -1 errno = %d err = \n", errno, strerror(errno));
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
        struct pidinfo *tinfo = pidSearch(&info->ptree,pid);
        if(!tinfo)
        {
//            dmsg("Unknown parent process\n");
            return;
        }
        switch (evtType) {
        case PTRACE_EVENT_FORK: // 进程组
            pidInsert(&info->ptree,createPidInfo(spid,spid,tinfo->gpid));
            break;
        case PTRACE_EVENT_VFORK: // 虚拟进程
            pidInsert(&info->ptree,createPidInfo(spid,spid,tinfo->gpid));
            break;
        case PTRACE_EVENT_CLONE: // 进程
            pidInsert(&info->ptree,createPidInfo(spid,tinfo->gpid,tinfo->ppid));
            break;
        default:
            break;
        }
    }
    else
        dmsg("PTRACE_GETEVENTMSG : %s(%d) pid is %d\n", strerror(errno),errno,spid);
}

static int getRelationalPid(const pid_t* pid, pid_t *gpid, pid_t *ppid)
{
    *gpid = 0;
    *ppid = 0;
    int ret = 0;
    DIR *dir = opendir("/proc");
    if(!dir) {DMSG(ML_WARN,"opendir : %s\n",strerror(errno)); return -1;}
    struct dirent *entry;
    char pidPath[64] = { 0 };
    while(entry = readdir(dir))
    {
        if(entry->d_name[0] == '.') continue;
        if(DT_DIR == entry->d_type)
        {
            sprintf(pidPath,"/proc/%s/task/%llu",entry->d_name,*pid);
            if(!access(pidPath,F_OK))
            {
                char *tmp = pidPath + strlen("/proc/");
                char *tmpend = strstr(tmp,"/");
                if(tmpend) tmpend[0] = '\0';
                else {ret = -2; break;}

                char *strend;
                *gpid = strtoll(tmp,&strend,10);
                if(strend == tmp) {*gpid = 0; ret = -3;}
                else ret = 0;
                tmpend[0] = '/';
                break;
            }
        }
    }
    closedir(dir);

    if(!ret)
    {
        // 获取 父进程 ID
        // 该过程并不修改ret的值
        // 因为当前ppid可有可无
        strcat(pidPath,"/status");
        FILE *fp = fopen(pidPath,"r");
        if(fp)
        {
            char buf[128] = {0};
            while(fgets(buf,sizeof(buf)-1,fp))
            {
                char *tmp = strstr(buf,"PPid:");
                if(!tmp) continue;
                tmp += strlen("PPid:");
                while(++tmp[0] == ' ');
                char *strend;
                *ppid = strtoll(tmp,&strend,10);
                if(strend == tmp) {*ppid = 0;}
                break;
            }
            fclose(fp);
        }
    }
    return ret;
}

typedef enum _WMRET
{
    WM_SUCC = 0,                     //处理成功
    WM_TARGET_PROCESS_EXIT = 1,      //进程退出
    WM_REGS_READ_ERROR = 2,          //寄存器读取失败
    WM_CALL_UNREASONABLE,
    WM_CALL_NOT_FOUND,           //容器中不存在对该调用的处理
    WM_IS_EVENT,                 //事件处理结束
    WM_IS_SIGNAL,                //信号处理结束
    WM_TO_BLOCK,                 //阻塞该系统调用
} WMRET;


static CbArgvs av;
WMRET workMain(pid_t *pid, int *status)
{
//    DMSG(ML_INFO,"pid is %llu status = %d\n",*pid,*status);
    /*注意这两个宏函数存在return的情况*/
    MANAGE_SIGNAL(*pid,*status,info);   /*信号处理*/
    MANAGE_EVENT(*pid,*status,info);    /*事件处理*/

    struct user user;
    long *regs = (long*)&user.regs;
    // 获取寄存器
    if(ptrace(PTRACE_GETREGS, *pid, 0, regs) < 0)
    {
        DMSG(ML_ERR,"PTRACE_GETREGS: %s(%d) %llu\n", strerror(errno),errno,*pid);
        return WM_REGS_READ_ERROR;
    }

//    if(CALL(regs) < 0) return WM_SUCC;  // 系统调用号存在等于-1的情况,原因未详细调查
    int callid = nDoS(CALL(regs));
    if(callid < 0 || callid >= CALL_MAX)    // 判断系统调用号在一个合理范围
        return WM_CALL_UNREASONABLE;
    if(callid == ID_EXIT_GROUP)             // 进程退出
        return WM_TARGET_PROCESS_EXIT;
    // 指针数组作为bloom使用，判断是否关注该系统调用
    if(IS_BEGIN(regs) ? !info->cbf[callid] : !info->cef[callid])
        return WM_CALL_NOT_FOUND;
//    DMSG(ML_INFO,"From *pid %d\tHit Call %d\n",*pid,callid);
    struct pidinfo *tinfo = pidSearch(&info->ptree,*pid);
    if(!tinfo)
    {
        pid_t gpid = 0, ppid = 0;
        if(!getRelationalPid(pid,&gpid,&ppid))
        {
            if(tinfo = createPidInfo(*pid,gpid,ppid))
                pidInsert(&info->ptree,tinfo);
        }
    }
    if(tinfo)
    {
        memset(&av,0,sizeof(CbArgvs));
//        av.block = ISBLOCK(info,callid);
        av.info = tinfo;
        av.cinfo = info;
        av.cctext.regs = regs;
//        av.task = task;
//        av.td = td;
        IS_BEGIN(regs) ? info->cbf[callid](&av): info->cef[callid](&av);
    }
    else
    {
        /*
         * tinfo = NULL 这种情况出现的场景可能是：
         * 一：
         * 上述代码既没有查询到info，又在创建info时失败了
         * 二：
         * 目标进程组A被监控前，新的可执行程序B已经被启动
         * 在这个进程B在退出时会通知进程组A，也会出现查询不到的情况
         * 无需关心该问题，进程组B已经被其它监控线程监控了
         */
        DMSG(ML_WARN,"Current *pid %d is not in ptree\n",*pid);
    }
    return WM_SUCC;
}

void onProcessTask(pid_t *pid, int *status)
{
    WMRET ret = workMain(pid,status);
//    if(ret != WM_CALL_NOT_FOUND) DMSG(ML_INFO,"workMain ret: %d\n", ret);
    switch (ret)
    {
    case WM_CALL_NOT_FOUND:
    case WM_IS_EVENT:       //事件直接放行
    case WM_SUCC:
    case WM_TO_BLOCK:
    case WM_REGS_READ_ERROR:
    case WM_CALL_UNREASONABLE:
        // 设置下次监控的类型
        if(ptrace(PTRACE_SETOPTIONS, *pid, NULL, EVENT_CONCERN) < 0)
            DMSG(ML_WARN,"PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),*pid);
        // 放行该任务(也可能是一个事件)
        if(ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0)
            DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
        break;
    case WM_IS_SIGNAL:    //部分信号直接放行
        // 继续该任务（信号）
        if(ptrace(PTRACE_CONT, *pid, 0, *status) < 0)
            DMSG(ML_WARN,"PTRACE_CONT : %s(%d) *pid is %d\n",strerror(errno),errno,*pid);
        break;
    case WM_TARGET_PROCESS_EXIT:
        /*
         * 进程退出
         * 两种退出形式，一种是正常退出 系统调用号(callid) = ID_EXIT_GROUP
         * 另一种是由于信号导致 ctrl + c || kill -9 || kill -15
         */
        // 取消对该进程的追踪，进入下一个循环
        // DMSG(ML_INFO,"*pid : %d to exit!\n",*pid);
        if(ptrace(PTRACE_DETACH, *pid, 0, 0) < 0)
            DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
        pidDelete(&info->ptree,*pid);
        break;
    default:
        DMSG(ML_WARN,"Unknown WMRET = %d\n",ret);
        break;
    }
}

void MonProcMain(pid_t cpid)
{
    setpriority(PRIO_PROCESS, getpid(), -20);
    do{
        info = calloc(1,sizeof(ControlInfo));
        if(!info)
        {
            DMSG(ML_ERR,"calloc fail errcode is %d, err is %s\n",errno,strerror(errno));
            break;
        }
        // 初始化监控信息
        initControlInfoCallBackInfo();
        // 开始监控
        if(!ptraceAttach(cpid))
            pidInsert(&info->ptree,createPidInfo(cpid,0,0));
        // 设置回调
        setTaskOptFunc(taskOpt);

        pid_t npid;
        int run=1,status;
        while(run)
        {
            status = 0;
            npid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0);
            if(npid == -1)      ifContinue();                       // 判断是否应该进入下个循环
            if(npid == cpid)    onControlThreadMsg(cpid,status);    // 这一般是来自主进程的控制信息
            else                onProcessTask(&npid,&status);       // 响应被监控进程反馈的事件
        }
    }while(0);
    return ;
}
