#include "workthread.h"
#include "workprocess.h"

static void getProcId(int eveType,pid_t pid,int status,struct _ControlInfo *info)
{
    return;
    pid_t spid;
    // 获取新进程的PID
    if(ptrace(PTRACE_GETEVENTMSG, pid, NULL, &spid) >= 0)
    {
        dmsg("Child process created: %d status = %d\n", spid, status);
        if(spid <= 0 ) return;
        struct pidinfo *tmpInfo = pidSearch(&info->ptree,pid);
        if(!tmpInfo)
        {
            dmsg("Unknown parent process\n");
            return;
        }
        switch (eveType) {
        case PTRACE_EVENT_FORK: // 进程组
            pidInsert(&info->ptree,createPidInfo(spid,spid,tmpInfo->gpid));
            break;
        case PTRACE_EVENT_VFORK: // 虚拟进程
            pidInsert(&info->ptree,createPidInfo(spid,spid,tmpInfo->gpid));
            break;
        case PTRACE_EVENT_CLONE: // 进程
            pidInsert(&info->ptree,createPidInfo(spid,tmpInfo->gpid,tmpInfo->ppid));
            break;
        default:
            break;
        }
    }
    else
        dmsg("PTRACE_GETEVENTMSG : %s(%d) pid is %d\n", strerror(errno),errno,spid);
}

static int getRelationalPid(const pid_t *pid, pid_t *gpid, pid_t *ppid)
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

enum APRET
{
    AP_SUCC = 0,                     //处理成功
    AP_TARGET_PROCESS_EXIT = 1,      //进程退出
    AP_REGS_READ_ERROR = 2,          //寄存器读取失败
    AP_CALL_NOT_FOUND = 3,           //容器中不存在对该调用的处理
    AP_IS_EVENT = 4,                 //事件处理结束
    AP_IS_SIGNAL = 5,                //信号处理结束
    AP_TO_BLOCK = 6,                 //阻塞该系统调用
};
// 分析预处理
enum APRET analysisPreproccess(ThreadData *td, Interactive *task, int *callid)
{
    pid_t *pid = &task->pid;
    int *status = &task->status;
    ControlInfo *info = td->cInfo;

    dmsg(" pid is %d\n",*pid);
    dmsg(">     status is %d     <\n",*status);
    /*注意这两个宏函数存在return的情况*/
//    MANAGE_SIGNAL(*pid,*status,info);
//    MANAGE_EVENT(*pid,*status,info);

//    struct user_regs_struct reg;
    if(ptrace(PTRACE_GETREGS, *pid, 0, task->regs) < 0)
    {
        DMSG(ML_ERR,"PTRACE_GETREGS: %s(%d)\n", strerror(errno),*pid);
//        return AP_REGS_READ_ERROR;
    }

    // printUserRegsStruct(&user.regs);
    long *pregs = task->regs;
    if(CALL(pregs) < 0) return AP_SUCC; // 系统调用号存在等于-1的情况,原因未详细调查
    *callid = nDoS(CALL(pregs));

    // 目标进程退出
    if(*callid == ID_EXIT_GROUP)
    {
        dmsg("Call is ID_EXIT_GROUP\n");
        return AP_TARGET_PROCESS_EXIT;
    }

    // 指针数组作为bloom使用，判断是否关注该系统调用
    if(IS_BEGIN(pregs) ? !info->cbf[*callid] : !info->cef[*callid])
        return AP_CALL_NOT_FOUND;
    DMSG(ML_INFO,"From pid %d\tHit Call %d\n",*pid,*callid);
    struct pidinfo *tmpInfo = pidSearch(&info->ptree,*pid);
    if(!tmpInfo)
    {
        pid_t gpid = 0, ppid = 0;
        if(!getRelationalPid(pid,&gpid,&ppid))
        {
            if(tmpInfo = createPidInfo(*pid,gpid,ppid))
                pidInsert(&info->ptree,tmpInfo);
        }
    }
    if(tmpInfo)
    {
        CbArgvs av;
        memset(&av,0,sizeof(CbArgvs));
//        av.block = ISBLOCK(info,*callid);
        av.info = tmpInfo;
//        av.task = task;
//        av.td = td;
        IS_BEGIN(pregs) ? info->cbf[*callid](&av): info->cef[*callid](&av);
    }
    else
    {
        /*
         * tmpInfo = NULL 这种情况出现的场景可能是：
         * 一：
         * 上述代码既没有查询到info，又在创建info时失败了
         * 二：
         * 目标进程组A被监控前，新的可执行程序B已经被启动
         * 在这个进程B在退出时会通知进程组A，也会出现查询不到的情况
         * 无需关心该问题，进程组B已经被其它监控线程监控了
         */
        DMSG(ML_WARN,"Current pid %d is not in ptree\n",*pid);
    }
    return AP_SUCC;
}

void *workThread(void* pinfo)
{
    sleep(2);
    ThreadData *td = pinfo;
    struct _ControlInfo *info = td->cInfo;

    int run = 1;
    int status = 0, callid = -1;
    pid_t pid = 0;

    struct user user;
    memset(&user,0,sizeof(struct user));
    Interactive task;
    memset(&task,0,sizeof(Interactive));
    task.regs = (long*)&user.regs;

    while(run)
    {
//        DMSG(ML_ERR,"<<<\n");
        pid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0);
//        DMSG(ML_ERR,">>> pid is %d\n",pid);
        // 判断pid
        if(pid == -1)
        {
            DMSG(ML_ERR,"pid = -1 errno = %d\n",errno);
            switch (errno)
            {
            case ECHILD:    // 没有被追踪的进程了，退出循环
                run = 0;
            case EINTR:     // 单纯被信号打断,接着wait
                continue;
            case EINVAL:    // 无效参数
            default:        // 其它错误
                run = 0;
                dmsg("%s\n",strerror(errno));
                continue;
            }
        }
        task.pid = pid;
        task.status = status;

        // 判断主线程是否要退出
        if(info->toexit)
        {            // 取消对该进程的追踪，进入下一个循环
            if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            pidDelete(&info->ptree,pid);
            continue;
        }

        enum APRET ret = analysisPreproccess(td,&task,&callid);    // 分析与初步处理
//        DMSG(ML_ERR,"analysisPreproccess ret is %d\n",ret);
        switch (ret)
        {
        case AP_SUCC:
        case AP_TO_BLOCK:
        case AP_IS_EVENT:     //事件直接放行
        case AP_CALL_NOT_FOUND:
            // 设置下次监控的类型
            if(ptrace(PTRACE_SETOPTIONS, pid, NULL, EVENT_CONCERN) < 0)
                DMSG(ML_WARN,"PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),pid);
            // 放行该任务(也可能是一个事件)
            if(ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            break;
        case AP_IS_SIGNAL:    //部分信号直接放行
            // 继续该任务（信号）
            if(ptrace(PTRACE_CONT, pid, 0, status) < 0)
                DMSG(ML_WARN,"PTRACE_CONT : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            break;
        case AP_TARGET_PROCESS_EXIT:
            /* 进程退出
             * 两种退出形式，一种是正常退出 系统调用号(callid) = ID_EXIT_GROUP
             * 另一种是由于信号导致 ctrl + c || kill -9 || kill -15
             */
            DMSG(ML_INFO,"pid : %d to exit!\n",pid);
            // 取消对该进程的追踪，进入下一个循环
            if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            pidDelete(&info->ptree,pid);
            break;
        default:
            DMSG(ML_WARN,"Unknown ANALYSISRET = %d\n",ret);
            break;
        }
    }
    return NULL;
}
