#include "sysmon.h"
int gSeize;                           // SEIZE模式与ATTACH模式
int gProcNum;                         // 用于进行系统监控的进程总数
int gPipeToMain[2];                   // 用于给主进程进行通讯的管道
int gPipeFromMain[2];                 // 用于从主进程获取信息的管道
struct rb_root gPidTree;              // 所监控的进程
InitInfo gInitInfo[PROC_MAX];         // 用于保存最初的初始化信息
ControlPolicy *gDefaultControlPolicy; // 全局默认控制策略
ControlPolicy *gCurrentControlPolicy; // 正在使用的控制策略（有时指向gDefaultControlInfo，有时指向进程自定义控制策略）
int iterateAllThreadsToProcs()
{
    pid_t *pids = NULL;
    if(iterateSysThreads(&pids))
        return -1;

    int skip;
    ManageInfo info;
    // 向'追踪进程'分配工作
    for(int i=0;pids[i];++i)
    {
        skip = 0;
        memset(&info,0,sizeof(ManageInfo));
        memcpy(info.tpfd,gInitInfo[i%gProcNum].cfd,sizeof(info.tpfd));
        info.type = MT_AddTid;
        info.tpid = pids[i];

        // 临时测试
        {
            if(info.tpid != 99145) skip = 1;
        }

        if(skip) continue;
        if(sendManageInfo(&info))
            DERR(sendManageInfo);
    }
    free(pids);
    return 0;
}

int printMsg(struct CbMsg *info);
int StartSystemMonitor()
{
    // 初始化寄存器偏移
    if(initRegsOffset())
        return -1;
    // 初始化主进程进出消息的管道
    pipe(gPipeToMain);
    pipe(gPipeFromMain);

    // 根据CPU核心数量初始化‘监控进程数量’
    gProcNum = sysconf(_SC_NPROCESSORS_ONLN);
    gProcNum = gProcNum > PROC_MAX ? PROC_MAX : gProcNum;
    gProcNum = 1;
    DMSG(ML_INFO, "Current cpu cont %d\n", gProcNum);

    // 定义'控制线程'所使用的匿名管道
    memset(&gInitInfo,0,sizeof(gInitInfo));
    // 创建‘监控进程’，并对gInitInfo初始化
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

            // 通知主进程，当前进程已经退出
            MData data;
            data.origin = MDO_MonProc;
            data.monproc.type = MPT_Exit;
            write(gDefaultControlPolicy->binfo.tpfd[1],&data,sizeof(data));
            exit(0);
        }
        else if(pid < 0)
        {
            DMSG(ML_ERR, "fork fail errcode %d, err is %s\n", errno, strerror(errno));
            exit(-1);
        }
        gInitInfo[i].spid = pid;
    }
    sleep(1);
    // 再一个循环，用来将ControlBaseInfo传递给各‘监控进程’
    int wbufsize = sizeof(ManageInfo)+sizeof(ControlBaseInfo);
    char *wbuf = calloc(1,wbufsize*gProcNum);
    for(int i=0;i<gProcNum;++i)
    {
        char *tmpaddr = wbuf+i*wbufsize;
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
        write(gInitInfo[i].cfd[1],tmpaddr,wbufsize);
        usleep(100);
    }
    free(wbuf);
    wbuf = NULL;
    // 遍历系统中所有的线程并对线程进行分配
    iterateAllThreadsToProcs();
    return 0;
}
