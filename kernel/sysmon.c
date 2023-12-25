#include "sysmon.h"

#define EVENT_CONCERN \
(PTRACE_O_TRACESYSGOOD|PTRACE_O_TRACEEXEC|\
PTRACE_O_TRACEEXIT|PTRACE_O_TRACECLONE|\
PTRACE_O_TRACEFORK|PTRACE_O_TRACEVFORK)

struct rb_root *cbTree = NULL;
pid_t contpid = 0;

struct threadArgs
{
    pid_t pid;
    struct rb_root *cbTree;
};

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

void *workThread(void* pinfo);
int filterGPid(const struct dirent *dir);
void* newStartMon(void* pinfo)
{
    ControlInfo *info = (ControlInfo *)pinfo;
    info->cpid = gettid();
    registerSignal();

    const char *directory = "/proc";
    struct dirent **namelist;
    int num_entries = scandir(directory, &namelist, filterGPid, alphasort);
    if (num_entries == -1) {
        perror("scandir");
        return NULL;
    }

    pid_t *tmp = NULL;
    pid_t pidss[1024] = {0};
    memset(&pidss,0,sizeof(pidss));
    size_t pidsslen = 0;

    FILE *fp = fopen("/tmp/gpids","w+");
    for (int i = 0; i < num_entries; ++i)
    {
        printf("%s\n", namelist[i]->d_name);
        pid_t gpid;
        char *strend;
        gpid = strtoll(namelist[i]->d_name,&strend,10);
        if(gpid && strend != namelist[i]->d_name)
        {
            if(gpid != 3091) continue;
//            // 获取进程组中的所有进程
            tmp = getTask(gpid);
            if(!tmp) return NULL;
            if(fp) fprintf(fp,"%d\n",gpid);

            pid_t *pids = tmp;
            // 对各个进程都建立追踪
            while(*pids)
            {
                printf("*pids = %d\n",*pids);
                pidss[pidsslen] = *pids;
                ++ pidsslen;
                ++ pids;
            }
            free(tmp);
            tmp = NULL;
        }
//        free(namelist[i]);
    }
    fclose(fp);


    int num = pidsslen;
    for(int i=0;i < (num > pidsslen ? pidsslen : num);++i)
    {
        if(pidss[i] == 0) continue;
        // 附加到被传入PID的进程
        if(ptrace(PTRACE_ATTACH, pidss[i], 0, 0) < 0)
        {
            dmsg("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,pidss[i]);
            if(errno == 3) continue;
            if(errno != 1) goto END;
        }
        // 添加进pid树
        if(errno != 1) pidInsert(&info->ptree,createPidInfo(pidss[i],0,0));
    }

    // 创建 epoll 实例
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        DMSG(ML_ERR,"epoll_create\n");
    }
    ThreadData *mtts[MAX_THREAD_NUM];
    struct epoll_event event[MAX_THREAD_NUM];
    memset(event,0,sizeof(event[MAX_THREAD_NUM]));
    for(int i=0;i<MAX_THREAD_NUM;++i)
    {
        ThreadData *mtt = calloc(1,sizeof(ThreadData));
        mtts[i] = mtt;
        // 初始化成员属性
        pipe(mtt->fd);
        mtt->cInfo = info;

        event[i].events = EPOLLIN;
        event[i].data.fd = mtt->fd[0];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event[i].data.fd, &(event[i])) == -1) {
            DMSG(ML_ERR,"epoll_ctl\n");
        }
        //

        //    waitTask(pinfo);
        pthread_t thread_id;
        pthread_attr_t  attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread_id, &attr, workThread, mtt);
        pthread_attr_destroy(&attr);
    }
#define MAX_EVENTS 10

    Interactive msg;
    // 准备用于接收事件的数组
    struct epoll_event events[MAX_EVENTS];
    // 循环：不断的处理工作线程请求的任务
    while(1)
    {
        // 等待事件发生
//        DMSG(ML_WARN,"wait <<<\n");
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
//        DMSG(ML_WARN,"wait >>>\n");
        if (num_events == -1) {
            DMSG(ML_ERR,"epoll_wait\n");
        }

        for (int i = 0; i < num_events; i++) {
            memset(&msg,0,sizeof(Interactive));
            DMSG(ML_INFO,"events[i] is %d\n",events[i]);
            ssize_t bytes_read = read(events[i].data.fd, &msg, sizeof(Interactive));
            if(bytes_read == -1)
            {
                DMSG(ML_ERR,"read(events[i].data.fd, &msg, sizeof(Interactive))\n");
                continue;
            }

            Interactive *curTask = &msg;
//            DMSG(ML_WARN,"TraceTaskContext curTask->type: %d\n", curTask->type);
            switch (curTask->type) {
            case TTT_GETREGS:
                if(ptrace(PTRACE_GETREGS, curTask->pid, 0, curTask->regs) < 0)
                {
                    DMSG(ML_ERR,"PTRACE_GETREGS: %s(%d)\n", strerror(errno),curTask->pid);
                    //                return AP_REGS_READ_ERROR;
                }
                write(curTask->fd[1],curTask,sizeof(Interactive));
//                DMSG(ML_WARN,"pthread_cond_signal(curTask->cond);\n");
                break;
            case TTT_SYSCALL:
//                DMSG(ML_WARN,"1curTask->pid is %d\n",curTask->pid);
                // 设置下次监控的类型
                if(ptrace(PTRACE_SETOPTIONS, curTask->pid, NULL, EVENT_CONCERN) < 0)
                    DMSG(ML_WARN,"PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),curTask->pid);
//                DMSG(ML_WARN,"2curTask->pid is %d\n",curTask->pid);
                // 放行该任务(也可能是一个事件)
                if(ptrace(PTRACE_SYSCALL, curTask->pid, 0, 0) < 0)
                    DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,curTask->pid);
//                DMSG(ML_WARN,"3curTask->pid is %d\n",curTask->pid);
                break;
            case TTT_CONT:
                // 继续该任务（信号）
                if(ptrace(PTRACE_CONT, curTask->pid, 0, curTask->status) < 0)
                    DMSG(ML_WARN,"PTRACE_CONT : %s(%d) pid is %d\n",strerror(errno),errno,curTask->pid);
                break;
            case TTT_PEEKDATA:
            {
                char *str = NULL;
                int *resg2Offset = (int*)&g_regsOffset + 2;
                for(int i=0;i<sizeof(curTask->argv)/sizeof(curTask->argv[0]);++i)
                {
                    if(curTask->argvType[i] == AT_STR)
                    {
                        if(getRegsStrArg(curTask->pid,curTask->regs[*(resg2Offset+i)],&str,&curTask->argvLen[i]))
                        {
                            DMSG(ML_WARN,"PTRACE_PEEKDATA error\n");
                        }
                        else
                            curTask->argv[i] = (long)str;
                    }
                }
                write(curTask->fd[1],curTask,sizeof(Interactive));
            }
            break;
            case TTT_SETREGS:
                if(ptrace(PTRACE_SETREGS, curTask->pid, NULL, curTask->regs) < 0)
                    DMSG(ML_WARN,"cbDoS error");
                break;
            case TTT_DETACH:
                // 取消追踪
                if(ptrace(PTRACE_DETACH, curTask->pid, 0, 0) < 0)
                    DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,curTask->pid);
                break;
            default:
                DMSG(ML_ERR,"ERROR Task Type : %d\n",curTask->type);
                break;
            }
        }
        if(info->toexit)    break;
    }

END:
    DMSG(ML_INFO,"Tree Size = %llu\n",pidTreeSize(&info->ptree));
    if(tmp) {free(tmp); tmp = NULL;}
    info->exit = 1;
    return NULL;
}



//void* waitTask(void* pinfo)
//{
//    ControlInfo *info = (ControlInfo *)pinfo;
//    pid_t pid = 0;
//    int status = 0,tocontrols = 0,
//        callid = 0,run = 1,block = 0,waitidret = 0;
//    siginfo_t siginfo;
//    memset(&siginfo,0,sizeof(siginfo_t));
//    while(run)
//    {
//        callid = 0;
//        status = 0;
//        tocontrols = 1;
//        dmsg("rewait >>>>>>>>>>>>>>>>>>>>>>\n");
//        // waitid(P_ALL, 存在监控其它并非自己ATTACH的进程组的问题，会导致逻辑混乱
//        // waitid(P_PGID, 存在不监控其它进程组的问题，需要另外拉起一个进程去等待新创建的进程组，否则新的进程组会一直阻塞
//        pid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0); //非阻塞 -> WNOHANG
//        // 判断pid
//        if(pid == -1)
//        {
//            DMSG(ML_ERR,"pid = -1 errno = %d\n",errno);
//            switch (errno) {
//            case ECHILD:    // 没有被追踪的进程了
//                run = 0;
//            case EINTR:     // 被信号打断
//                continue;
//                break;
//            case EINVAL:    // 无效参数
//            default:        // 其它错误
//                run = 0;
//                dmsg("%s\n",strerror(errno));
//                continue;
//                break;
//            }
//        }

//        // 主线程通知结束
//        if(info->toexit)
//        {
//            printf("detach pid = %d\n",pid);
//            if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
//                DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
//            continue;
//        }

//        // 开始处理
//        if(siginfo.si_pid && siginfo.si_status)
//        {
//            enum APRET ret =
//                analysisPreproccess(&pid,&status,,info,&callid);        // 分析与初步处理
//            switch (ret) {
//            case AP_SUCC:
//                tocontrols = 1;
//                break;
//            case AP_TARGET_PROCESS_EXIT:
//                /* 进程退出
//                 *
//                 * 两种退出形式，一种是正常退出 系统调用号(callid) = ID_EXIT_GROUP
//                 * 另一种是由于信号导致 ctrl + c || kill -9 || kill -15
//                 */
//                tocontrols = 0;
//                DMSG(ML_INFO,"pid : %d to exit!\n",pid);
//                // 取消对该进程的追踪，进入下一个循环
//                if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
//                    DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
//                pidDelete(&info->ptree,pid);
//                continue;
//                break;
//            case AP_IS_EVENT:     //事件已经被直接放行了,故直接continue
//            case AP_IS_SIGNAL:    //部分信号已经被直接放行了,故直接continue
//                continue;
//            case AP_CALL_NOT_FOUND:
//                dmsg("Syscall:%d doesn't exist in callback bloom!\n",callid);
//                break;
//            case AP_TO_BLOCK:
//                block = 1;
//                break;
//            default:
//                DMSG(ML_WARN,"Unknown ANALYSISRET = %d\n",ret);
//                break;
//            }
//            if(tocontrols) controls(&pid,&status,&info->block[callid]);        // 处理
//        }
//        // 继续该事件
//        if(!block && ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) {
//            DMSG(ML_WARN,"%s : %s(%d) pid is %d\n", "PTRACE_SYSCALL", strerror(errno),errno,pid);
//        }
//    }
//    return NULL;
//}

//void* startMon(void* pinfo)
//{
//    ControlInfo *info = (ControlInfo *)pinfo;
//    info->cpid = gettid();
//    registerSignal();
//
//    // 获取进程组中的所有进程
//    pid_t *tmp = getTask(info->tpid);
//    if(!tmp) return NULL;
//    pid_t *pids = tmp;
//    // 对各个进程都建立追踪
//    while(*pids)
//    {
//        DMSG(ML_INFO,"startMon pid is %d\n",*pids);
//        // 附加到被传入PID的进程
//        if(ptrace(PTRACE_ATTACH, *pids, 0, 0) < 0)
//        {
//            dmsg("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,*pids);
//            goto END;
//        }
//        // 添加进pid树
//        pidInsert(&info->ptree,createPidInfo(*pids,info->tpid,0));
//        ++ pids;
//    }
//    free(tmp);
//    tmp = NULL;
//
//
//    pid_t pid = 0;
//    int status = 0,tocontrols = 0,
//        callid = 0,run = 1,block = 0;
//    while(run)
//    {
//        callid = 0;
//        status = 0;
//        tocontrols = 1;
//        dmsg("rewait >>>>>>>>>>>>>>>>>>>>>>");
//        pid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0); //非阻塞 -> WNOHANG
//
//        // 判断pid
//        if(pid == -1)
//        {
//            switch (errno) {
//            case ECHILD:    // 没有被追踪的进程了
//                run = 0;
//            case EINTR:     // 被信号打断
//                continue;
//                break;
//            case EINVAL:    // 无效参数
//            default:        // 其它错误
//                run = 0;
//                dmsg("%s\n",strerror(errno));
//                continue;
//                break;
//            }
//        }
//
//        // 主线程通知结束
//        if(info->toexit)
//        {
//            printf("detach pid = %d\n",pid);
//            if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
//                DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
//            continue;
//        }
//
//        // 开始处理
//        if(pid && status)
//        {
//            enum APRET ret =
//                analysisPreproccess(&pid,&status,info,&callid);        // 分析与初步处理
//            switch (ret) {
//            case AP_SUCC:
//                tocontrols = 1;
//                break;
//            case AP_TARGET_PROCESS_EXIT:
//                /* 进程退出
//                 *
//                 * 两种退出形式，一种是正常退出 系统调用号(callid) = ID_EXIT_GROUP
//                 * 另一种是由于信号导致 ctrl + c || kill -9 || kill -15
//                 */
//                tocontrols = 0;
//                DMSG(ML_INFO,"pid : %d to exit!\n",pid);
//                // 取消对该进程的追踪，进入下一个循环
//                if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
//                    DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
//                pidDelete(&info->ptree,pid);
//                continue;
//                break;
//            case AP_IS_EVENT:     //事件已经被直接放行了,故直接continue
//            case AP_IS_SIGNAL:    //部分信号已经被直接放行了,故直接continue
//                continue;
//            case AP_CALL_NOT_FOUND:
//                dmsg("Syscall:%d doesn't exist in callback bloom!\n",callid);
//                break;
//            case AP_TO_BLOCK:
//                block = 1;
//                break;
//            default:
//                DMSG(ML_WARN,"Unknown ANALYSISRET = %d\n",ret);
//                break;
//            }
//            if(tocontrols) controls(&pid,&status,&info->block[callid]);        // 处理
//        }
//        // 继续该事件
//        if(!block && ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) {
//            DMSG(ML_WARN,"%s : %s(%d) pid is %d\n", "PTRACE_SYSCALL", strerror(errno),errno,pid);
//        }
//    }
//
//END:
//    DMSG(ML_INFO,"Tree Size = %llu\n",pidTreeSize(&info->ptree));
//    if(tmp) {free(tmp); tmp = NULL;}
//    info->exit = 1;
//    return NULL;
//}
