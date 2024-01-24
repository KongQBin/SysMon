#include "managethread.h"
static int procInited = 0;
void (*taskOptFunc)(ManageInfo *minfo,ControlBaseInfo *cbinfo) = NULL;
void setTaskOptFunc(void *func)
{
    if(!taskOptFunc) taskOptFunc = func;
}
// 1： 要对哪个进程进行拷贝
// 2： 位于目标进程的起始地址
// 3： 要拷贝到的目标地址
// 4： 要拷贝的长度
// 如果参数4=NULL,则表示要拷贝的是字符串，此时参数3将被自动开辟
int getArg(pid_t *pid, long *originaddr, void **targetaddr, long *len)
{
    if(len)
    {
        long readLen = *len;
        for(int i=0;i<*len/WORDLEN+1;++i)
        {
            long tmpbuf = ptrace(PTRACE_PEEKDATA, *pid, *originaddr + (i*WORDLEN), NULL);
            memcpy(&((long*)*targetaddr)[i],&tmpbuf,(readLen > WORDLEN) ? WORDLEN : readLen);
            readLen -= WORDLEN;
        }
    }
    else
    {
//        for(int i=0;;++i)
//        {

//        }
    }
    return 0;
}

static ManageInfo *minfo;
void onControlThreadMsg(pid_t pid, int status)
{
    do
    {
        int toBreak = 0;
        if(WIFSIGNALED(status))     /*kill -9)*/
        {
            DMSG(ML_INFO,"WIFSIGNALED exit signal is %d\n",WTERMSIG(status));
            if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            break;
        }
        int signal = (WSTOPSIG(status) & 0x7F);
        switch (signal) {
        case SIGTERM:               /* kill -15 */
        case SIGINT:                /* 2 Ctrl + c */
            if(ptrace(PTRACE_CONT, pid, 0, status) < 0)
                DMSG(ML_WARN,"PTRACE_CONT : %s(%d) pid is %d\n",strerror(errno),errno,pid);
            toBreak = 1;
            break;
        case SIGCHLD:               /* 17 子进程的退出或终止事件 */
        case SIGTRAP:               /* kill -5 */
        default:
            //        DMSG(ML_WARN,"Unknown signal %d\n",signal);
            break;
        }
        if(toBreak) break;      // signal

        do
        {
            struct user user;
            memset(&user,0,sizeof(struct user));
            long *regs = (long*)&user.regs;
            if(ptrace(PTRACE_GETREGS, pid, 0, regs) < 0)
                DMSG(ML_ERR,"PTRACE_GETREGS: %s(%d)\n", strerror(errno),pid);

            int callid = nDoS(CALL(regs));
            if(callid == ID_EXIT_GROUP)
            {
                if(ptrace(PTRACE_DETACH, pid, 0, 0) < 0)
                    DMSG(ML_WARN,"PTRACE_DETACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
                toBreak = 1;
                break;
            }

            if(CALL(regs) < 0 || callid != ID_WRITE)
                break;
            if(!taskOptFunc)
                break;
            if(ARGV_1(regs) < MT_Start || ARGV_1(regs) > MT_End)
                break;
            if(ARGV_3(regs) != sizeof(ManageInfo) && ARGV_3(regs) != sizeof(ManageInfo)+sizeof(ControlBaseInfo))
                break;

            if(IS_BEGIN(regs))
            {
                if(!minfo) minfo = calloc(1,sizeof(ManageInfo));
                else memset(minfo,0,sizeof(ManageInfo));
                if(!minfo) break;
                // 获取任务
                ControlBaseInfo *cbinfo = NULL;
                long readLen = sizeof(ManageInfo);
                long originaddr = ARGV_2(regs);
                getArg(&pid,&originaddr,(void**)&minfo,&readLen);
                if(minfo->type == MT_Init)
                {
                    cbinfo = calloc(1,sizeof(ControlBaseInfo));
                    if(cbinfo)
                    {
                        // 初始化 ControlInfo::ControlBaseInfo;
                        DMSG(ML_INFO,"Init ControlBaseInfo\n");
                        readLen = sizeof(ControlBaseInfo);
                        originaddr = ARGV_2(regs) + sizeof(ManageInfo);
                        getArg(&pid,&originaddr,(void**)&cbinfo,&readLen);

//                        DMSG(ML_INFO,"cbinfo->bnum %d\n",cbinfo->bnum);
//                        for(int i=0;i<cbinfo->bnum;++i)
//                        {
//                            DMSG(ML_INFO," -- > bpids[%d] = %d\n",i,cbinfo->bpids[i]);
//                        }
                    }
                }
                CALL(regs) = CALL(regs) | (1 << (WORDLEN*sizeof(int)-1));
                taskOptFunc(minfo,cbinfo);
                if(cbinfo) {free(cbinfo);cbinfo=NULL;}
            }
            else
                RET(regs) = -1;
            // 设置寄存器
            if(ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0)
                DMSG(ML_WARN,"PTRACE_SETREGS : %s(%d) pid is %d\n",strerror(errno),errno,pid);
        }while(0);
        if(!toBreak)
        {
            // 放行该任务
            if(ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,pid);
        }
    }while(0);
    return;
}


// 向'控制线程' manageThreadFunc 发送消息
int sendManageInfo(ManageInfo *info)
{
    return write(info->tpfd[1],info,sizeof(ManageInfo)) != sizeof(ManageInfo);
}
// 此处不要被write误导，该write调用会被‘监控进程’所解析
// 是利用write调用来与‘追踪进程’进行消息传递,达到控制的目的
// 因为‘监控进程’利用wait4在进行监控，如果利用信号去打断它的话
// 担心有可能会丢失‘被监控进程’的事件，进而导致一些难以控制的问题
// 故使用该线程以被追踪的方法来向‘监控进程’传递控制信息
// 该线程内不要有过多的逻辑，保持逻辑简洁，以保证不会导致‘监控进程’误判
#define SendMsg(wfd,addr,len)   write(wfd,addr,len);
void* manageThreadFunc(void *val)
{
    InitInfo *info = val;
    info->pid = gettid();
    // 当前线程是被'追踪进程'进行追踪的
    // 故不要有太多的系统调用逻辑，以免影响'追踪进程'的性能
    int exitNum = 0;
    int readfd = info->cfd[0];

    int wbuflen,wbufbaselen = sizeof(ManageInfo);
    char *wbuf = calloc(1, wbufbaselen+sizeof(ControlBaseInfo));
    ManageInfo *minfo = (ManageInfo *)wbuf;
    ControlBaseInfo *cbinfo = (ControlBaseInfo *)(minfo+1);
    while(1)
    {
        wbuflen = wbufbaselen;
        memset(minfo,0,wbufbaselen);
        read(readfd,minfo,wbuflen);

        if(minfo->type == MT_Init)
        {
            memset(cbinfo,0,sizeof(ControlBaseInfo));
            read(readfd,cbinfo,sizeof(ControlBaseInfo));
            wbuflen += sizeof(ControlBaseInfo);
        }
        SendMsg(minfo->type,wbuf,wbuflen);
        // 第一遍退出是通知监控进程退出,第二遍退出是退出当前线程
        if(minfo->type == MT_ToExit && ++exitNum >= 2)   break;
    }
    if(wbuf) free(wbuf);
    return NULL;
}

pthread_t createManageThread(InitInfo *info)
{
    int ret;
    pthread_t tid;
    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    if(ret)
    {
        DMSG(ML_ERR, "pthread_attr_init fail errcode is %d, err is %s\n", errno, strerror(errno));
        exit(1);
    }
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&tid, &attr, manageThreadFunc, info);
    if(ret)
    {
        DMSG(ML_ERR, "pthread_create fail errcode is %d, err is %s\n", errno, strerror(errno));
        exit(1);
    }
    return tid;
}
