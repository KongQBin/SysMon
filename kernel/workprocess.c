#include "workprocess.h"
static ControlInfo *info;
int ptrace_attach(pid_t pid)
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

int initControlInfoCallBackInfo()
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

void OnControlThreadMsg(pid_t pid, int status)
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
            if(status >> 16)    // event
            {
                DMSG(ML_INFO,"event is  %d\n",status >> 16);
                break;
            }

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
            if(ARGV_3(regs) != sizeof(ManageInfo) && ARGV_3(regs) != sizeof(ManageInfo)+sizeof(ControlBaseInfo))
                break;

            if(IS_BEGIN(regs))
            {
                // 获取任务
                ManageInfo minfo;
                memset(&minfo,0,sizeof(minfo));
                int readLen = sizeof(minfo);
                for(int i=0;i<sizeof(minfo)/WORDLEN+1;++i)
                {
                    long tmpbuf = ptrace(PTRACE_PEEKDATA, pid, ARGV_2(regs) + (i*WORDLEN), NULL);
                    memcpy(&((long*)&minfo)[i],&tmpbuf,(readLen > WORDLEN) ? WORDLEN : readLen);
                    readLen -= WORDLEN;
                }
                if(minfo.type == MT_Init)
                {
                    // 初始化 ControlInfo::ControlBaseInfo;
                    DMSG(ML_INFO,"Init ControlBaseInfo\n");
                    readLen = sizeof(ControlBaseInfo);
                    for(int i=0;i<sizeof(ControlBaseInfo)/WORDLEN+1;++i)
                    {
                        long tmpbuf = ptrace(PTRACE_PEEKDATA, pid, ARGV_2(regs)+sizeof(minfo)+(i*WORDLEN), NULL);
                        memcpy(&((long*)&info->binfo)[i],&tmpbuf,(readLen > WORDLEN) ? WORDLEN : readLen);
                        readLen -= WORDLEN;
                    }

                    DMSG(ML_INFO,"info->bnum %d\n",info->binfo.bnum);
                    for(int i=0;i<info->binfo.bnum;++i)
                    {
                        DMSG(ML_INFO," -- > bpids[%d] = %d\n",i,info->binfo.bpids[i]);
                    }
                }
                CALL(regs) = CALL(regs) | (1 << (WORDLEN*sizeof(int)-1));
            }
            else
                RET(regs) = -1;
            // 设置寄存器
            if(ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0)
                DMSG(ML_WARN,"PTRACE_SETREGS : %s(%d) pid is %d\n",strerror(errno),errno,pid);
        }while(0);
        if(!toBreak)
        {
            // 设置下次监控的类型
            if(ptrace(PTRACE_SETOPTIONS, pid, NULL, EVENT_CONCERN) < 0)
                DMSG(ML_WARN,"PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),pid);
            // 放行该任务(也可能是一个事件)
            if(ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,pid);
        }
    }while(0);
    return;
}

int IfContinue()
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
        dmsg("%s\n",strerror(errno));
        break;
    }
    return ret;
}

void MonProcMain(pid_t cpid)
{
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
        if(!ptrace_attach(cpid)) pidInsert(&info->ptree,createPidInfo(cpid,0,0));

        int run = 1;
        int status;
        struct user user;
        memset(&user,0,sizeof(struct user));
        while(run)
        {
            status = 0;
            pid_t npid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0);
            if(npid == -1)   IfContinue();                       // 判断是否应该进入下个循环
            if(npid == cpid)  OnControlThreadMsg(cpid,status);     // 这一般是来自主进程的控制信息
        }

        for(int i=0;i<info->binfo.bnum;++i)
        {
            DMSG(ML_INFO,"bpids[%d] = %d\n",i,info->binfo.bpids[i]);
        }

    }while(0);
    sleep(100);
    return ;
}
