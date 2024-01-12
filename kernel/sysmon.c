#include "sysmon.h"

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
        if (de->d_fileno == 0 || !de->d_name) continue;
        char *endptr;
        pid_t tpid = strtoll(de->d_name,&endptr,10);
        if(de->d_name == endptr || tpid <= 0) continue;    //转换失败||非法的pid
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
//int startSysmon(ControlInfo *info)
//{
//    info->cpid = gettid();
//    registerSignal();

//    const char *directory = "/proc";
//    struct dirent **namelist;
//    int num_entries = scandir(directory, &namelist, filterGPid, alphasort);
//    if (num_entries == -1) {
//        perror("scandir");
//        return -1;
//    }

//    pid_t *tmp = NULL;
//    pid_t pidss[1024] = {0};
//    memset(&pidss,0,sizeof(pidss));
//    size_t pidsslen = 0;

//    FILE *fp = fopen("/tmp/gpids","w+");
//    for (int i = 0; i < num_entries; ++i)
//    {
//        printf("%s\n", namelist[i]->d_name);
//        pid_t gpid;
//        char *strend;
//        gpid = strtoll(namelist[i]->d_name,&strend,10);
//        if(gpid && strend != namelist[i]->d_name)
//        {
//            if(gpid != 42267) continue;
////            // 获取进程组中的所有进程
//            tmp = getTask(gpid);
//            if(!tmp) return -2;
//            if(fp) fprintf(fp,"%d\n",gpid);

//            pid_t *pids = tmp;
//            // 对各个进程都建立追踪
//            while(*pids)
//            {
//                printf("*pids = %d\n",*pids);
//                pidss[pidsslen] = *pids;
//                ++ pidsslen;
//                ++ pids;
//            }
//            free(tmp);
//            tmp = NULL;
//        }
////        free(namelist[i]);
//    }
//    fclose(fp);


//    int num = pidsslen;
//    for(int i=0;i < (num > pidsslen ? pidsslen : num);++i)
//    {
//        if(pidss[i] == 0) continue;
//        // 附加到被传入PID的进程
//        if(ptrace(PTRACE_ATTACH, pidss[i], 0, 0) < 0)
//        {
//            dmsg("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,pidss[i]);
//            if(errno == 3) continue;
//            if(errno != 1) goto END;
//        }
//        // 添加进pid树
//        if(errno != 1) pidInsert(&info->ptree,createPidInfo(pidss[i],0,0));
//    }

//    ThreadData mtt;
//    // 初始化成员属性
//    mtt.cInfo = info;
//    workThread(&mtt);

//END:
//    DMSG(ML_INFO,"Tree Size = %llu\n",pidTreeSize(&info->ptree));
//    if(tmp) {free(tmp); tmp = NULL;}
//    info->exit = 1;
//    return 0;
//}

int mreadlinkFilter(char *originPath)
{
    int ret = 0;
    size_t mlen = 0, olen = 0;
    char *targetPath = NULL;
    while(1)
    {
        targetPath = calloc(1,mlen + 256);
        if(!targetPath) {ret = 1; break;}
        olen = mlen + 256;
        mlen = readlink(originPath, targetPath, olen);
        if(!targetPath) {ret = 1; break;}
        if(mlen < olen) break;
        else {free(targetPath);targetPath = NULL;}
    }
    if(!ret && targetPath)
    {
        do{
            // 内核进程没有exe
            if(!strlen(targetPath))
            {
                ret = 1;
                break;
            }

            // Xorg但凡使用图形化就会刷新，也不监控
            char *index = NULL;
            if((index = strstr(targetPath,"Xorg")) != NULL)
            {
                if((index + strlen("Xorg"))[0] == '\0')
                {
                    ret = 1;
                    break;
                }
            }
        }while(0);
    }
    if(targetPath) free(targetPath);
    return ret;
}

// 过滤进程组
int filterGPid(const struct dirent *dir)
{
    int ret = 0;
    pid_t gpid = 0;
    char *strend = NULL;
    char statusPath[64] = { 0 };
    do
    {
        if(dir->d_type != DT_DIR)                           break;  // 非目录
        snprintf(statusPath,sizeof(statusPath)-1,"/proc/%s/exe",dir->d_name);
        if(access(statusPath,F_OK))                         break;  // 目录中不包含exe文件
        if(mreadlinkFilter(statusPath))                     break;  // 进一步过滤
        gpid = strtoll(dir->d_name,&strend,10);
        if(dir->d_name != strend && (gpid == getpid()))     break;  // 转换成功但pid等于自身
        ret = 1;
    }while(0);
    return ret;
}



void MonProcDataOption(MonProc *mpdata)
{

}
void OutsideDataOption(Outside *odata)
{
    switch (odata->type) {
    case OT_AdmMon:
        sendManageInfo(&odata->info);
        break;
    case OT_AdmMain:
        break;
    default:
        break;
    }
}


int gProcNum;
int gPipeToMain[2];             // 用于给主进程进行通讯的管道
int gPipeFromMain[2];           // 用于从主进程获取信息的管道
InitInfo gInitInfo[PROC_MAX];   // 用于保存最初的初始化信息

int printMsg(struct CbMsg *info);
int StartSystemMonitor()
{
    // 初始化寄存器偏移
    initRegsOffset_f();
    // 初始化主进程进出消息的管道
    pipe(gPipeToMain);
    pipe(gPipeFromMain);

    // 根据CPU核心数量初始化‘监控进程数量’
    int gProcNum = sysconf(_SC_NPROCESSORS_ONLN);
    gProcNum = gProcNum > PROC_MAX ? PROC_MAX : gProcNum;
    DMSG(ML_INFO, "Current cpu cont %d\n", gProcNum);

    // 定义'控制线程'所使用的匿名管道
    memset(&gInitInfo,0,sizeof(gInitInfo));
    // 根据核心数量创建‘监控进程’
    for(int i=0; i<gProcNum; ++i)
    {
        pipe(gInitInfo[i].cfd);
        // 创建‘控制线程’并初始化
        gInitInfo[i].tid = createManageThread(&gInitInfo[i]);
        while(!gInitInfo[i].pid) usleep(1);/*等待初始化*/
        DMSG(ML_INFO,"Manage thread pid %lld\n",gInitInfo[i].pid);

        pid_t pid = fork();
        if(pid == 0)
        {
            // 设置回调函数
            PutMsg = printMsg;
            MonProcMain(gInitInfo[i].pid);
            exit(0);
        }
        else if(pid < 0)
        {
            DMSG(ML_ERR, "fork fail errcode %d, err is %s\n", errno, strerror(errno));
            exit(-1);
        }
        gInitInfo[i].spid = pid;
    }

    sleep(2);
    // 再一个循环，用来将ControlInfo传递给各‘监控进程’
    int wbuflen = sizeof(ManageInfo)+sizeof(ControlBaseInfo);
    char *wbuf = calloc(1,wbuflen*gProcNum);
    for(int i=0;i<gProcNum;++i)
    {
        char *tmpaddr = wbuf+i*wbuflen;
        ManageInfo *minfo = (ManageInfo *)(tmpaddr);
        ControlBaseInfo *cbinfo = (ControlBaseInfo *)(minfo+1);
        minfo->type = MT_Init;
        cbinfo->pid = gInitInfo[i].spid;
        cbinfo->mainpid = gettid();
        cbinfo->bnum = gProcNum;
        for(int j=0;j<gProcNum;++j)
        {
            cbinfo->bpids[j] = gInitInfo[j].spid;
        }
        memcpy(cbinfo->tpfd,gPipeToMain,sizeof(gPipeToMain));
        write(gInitInfo[i].cfd[1],tmpaddr,wbuflen);
        usleep(100);
    }
    sleep(100);
    // 遍历系统中所有的线程并对线程进行分配
    MData data;
    while(1)
    {
        memset(&data,0,sizeof(data));
        read(gPipeToMain[0],&data,sizeof(data));
        switch (data.origin) {
        case MDO_MonProc:
            MonProcDataOption(&data.monproc);
            break;
        case MDO_Outside:
            OutsideDataOption(&data.outside);
            break;
        default:
            DMSG(ML_ERR,"Unknown MData Origin is %d\n",data.origin);
            break;
        }
    }
    return 0;
}
