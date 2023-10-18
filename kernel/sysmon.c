#include "sysmon.h"

#define EVENT_CONCERN \
(PTRACE_O_TRACESYSGOOD|PTRACE_O_TRACEEXEC|\
PTRACE_O_TRACEEXIT|PTRACE_O_TRACECLONE|\
PTRACE_O_TRACEFORK|PTRACE_O_TRACEVFORK)

void getProcId(int eveType,pid_t pid,int status,struct ControlInfo *info)
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

int getRelationalPid(const pid_t *pid, pid_t *gpid, pid_t *ppid)
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

struct rb_root *cbTree = NULL;
pid_t contpid = 0;

struct threadArgs
{
    pid_t pid;
    struct rb_root *cbTree;
};

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
enum APRET analysisPreproccess(pid_t *pid,int *status,struct ControlInfo *info, int *callid)
{
    dmsg(" pid is %d\n",*pid);
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
        DMSG(ML_ERR,"PTRACE_GETREGS: %s(%d)\n", strerror(errno),*pid);
        return AP_REGS_READ_ERROR;
    }

    // printUserRegsStruct(&reg);
    long *pregs = (long*)&reg;
    if(CALL(pregs) < 0) return AP_SUCC; // 系统调用号存在等于-1的情况,原因未详细调查
    *callid = nDoS(CALL(pregs));

    // 目标进程退出
    if(*callid == ID_EXIT_GROUP)
    {
        dmsg("Call is ID_EXIT_GROUP\n");
        return AP_TARGET_PROCESS_EXIT;
    }
    // 指针数组作为bloom使用，判断是否关注该系统调用
    if(IS_BEGIN(pregs) ?
            !info->cbf[*callid] :
            !info->cef[*callid])
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
        IS_BEGIN(pregs) ?
            info->cbf[*callid](tmpInfo,pregs,ISBLOCK(info,*callid)):
            info->cef[*callid](tmpInfo,pregs,ISBLOCK(info,*callid));
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

pid_t *getTask(pid_t gpid)
{
    char taskPath[64] = { 0 };
    sprintf(taskPath,"/proc/%llu/task",gpid);
    DIR *dir = opendir(taskPath);
    if(!dir) return NULL;

    int pidsize = 10;
    pid_t *pids = calloc(1,pidsize*sizeof(pid_t)); // 先开辟10个
    if(!pids) return pids;

    pid_t *index = pids;
    struct dirent *de;
    while((de = readdir(dir)) != NULL)
    {
        if (de->d_fileno == 0)
            continue;
        if(!de->d_name)
            continue;
        char *endptr;
        pid_t tpid = strtoll(de->d_name,&endptr,10);
        if(de->d_name == endptr)    //转换失败
            continue;
        if(tpid <= 0)               //非法的pid
            continue;
        *index = tpid;
        ++index;
        if(index == pids+pidsize)
        {
            pid_t *tmp = realloc(pids,(pidsize+10)*sizeof(pid_t));
            if(tmp)
            {
                pids = tmp;
                pidsize += 10;
                memset(index, 0, 10*sizeof(pid_t));
            }
            else
            {
                dmsg("getTask :: realloc error : %s\n",strerror(errno));
                break;
            }
        }
    }

    if(labs(pids - index) == pidsize*sizeof(pid_t)) //正好装满
    {
        pid_t *tmp = realloc(pids,pidsize+1);
        if(tmp)
        {
            pids = tmp;
            pidsize += 1;
        }
        else
            dmsg("getTask :: end realloc error : %s\n",strerror(errno));
    }
    // 最后一个作为结束符号
    // 注意如果中途realloc失败会丢失最后一个元素
    *index = 0;
    return pids;
}



int filterGPid(const struct dirent *dir);
void* startMon(void* pinfo)
{
    struct ControlInfo *info = (struct ControlInfo *)pinfo;
    info->cpid = gettid();
    registerSignal();

    // 获取进程组中的所有进程
    pid_t *tmp = getTask(info->tpid);
    if(!tmp) return NULL;
    pid_t *pids = tmp;
    // 对各个进程都建立追踪
    while(*pids)
    {
        DMSG(ML_ERR,"startMon pid is %d\n",*pids);
        // 附加到被传入PID的进程
        if(ptrace(PTRACE_ATTACH, *pids, 0, 0) < 0)
        {
            dmsg("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,*pids);
            goto END;
        }
        // 添加进pid树
        pidInsert(&info->ptree,createPidInfo(*pids,info->tpid,0));
        ++ pids;
    }
    free(tmp);
    tmp = NULL;


    pid_t pid = 0;
    int status = 0,tocontrols = 0,
        callid = 0,run = 1,block = 0;
    while(run)
    {
        callid = 0;
        status = 0;
        tocontrols = 1;
        dmsg("rewait >>>>>>>>>>>>>>>>>>>>>>\n");
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

        // 主线程通知结束
        if(info->toexit)
        {
            printf("detach pid = %d\n",pid);
            if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            continue;
        }

        // 开始处理
        if(pid && status)
        {
            enum APRET ret =
                analysisPreproccess(&pid,&status,info,&callid);        // 分析与初步处理
            switch (ret) {
            case AP_SUCC:
                tocontrols = 1;
                break;
            case AP_TARGET_PROCESS_EXIT:
                /* 进程退出
                 *
                 * 两种退出形式，一种是正常退出 系统调用号(callid) = ID_EXIT_GROUP
                 * 另一种是由于信号导致 ctrl + c || kill -9 || kill -15
                 */
                tocontrols = 0;
                DMSG(ML_INFO,"pid : %d to exit!\n",pid);
                // 取消对该进程的追踪，进入下一个循环
                if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
                    DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
                pidDelete(&info->ptree,pid);
                continue;
                break;
            case AP_IS_EVENT:     //事件已经被直接放行了,故直接continue
            case AP_IS_SIGNAL:    //部分信号已经被直接放行了,故直接continue
                continue;
            case AP_CALL_NOT_FOUND:
                dmsg("Syscall:%d doesn't exist in callback bloom!\n",callid);
                break;
            case AP_TO_BLOCK:
                block = 1;
                break;
            default:
                DMSG(ML_WARN,"Unknown ANALYSISRET = %d\n",ret);
                break;
            }
            if(tocontrols) controls(&pid,&status,&info->block[callid]);        // 处理
        }
        // 继续该事件
        if(!block && ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) {
            DMSG(ML_WARN,"%s : %s(%d) pid is %d\n", "PTRACE_SYSCALL", strerror(errno),errno,pid);
        }
    }

END:
    DMSG(ML_INFO,"Tree Size = %llu\n",pidTreeSize(&info->ptree));
    if(tmp) {free(tmp); tmp = NULL;}
    info->exit = 1;
    return NULL;
}
