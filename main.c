#include <pthread.h>
#include "sysmon.h"

extern pid_t contpid;
extern pthread_t thread_id;

struct ControlInfo *ginfo = NULL;
int createMonThread(pid_t pid)
{
    struct ControlInfo *info = calloc(1,sizeof(struct ControlInfo));
    if(!info)
    {
        DMSG(ML_ERR, "Create ControlInfo err : %s\n",strerror(errno));
        return -1;
    }

    // 设置被监控的ID
    info->tpid = pid;
    // 设置监控到关注的系统调用，在其执行前后的调用函数
    SETFUNC(info,ID_WRITE,cbWrite,ceWrite);
    SETFUNC(info,ID_FORK,cbFork,ceFork);
    SETFUNC(info,ID_CLONE,cbClone,ceClone);
    SETFUNC(info,ID_EXECVE,cbExecve,ceExecve);
    SETFUNC(info,ID_CLOSE,cbClose,ceClose);
    SETFUNC(info,ID_OPENAT,cbOpenat,ceOpenat);
    // 设置阻塞模式
//    SETBLOCK(info,ID_EXECVE);

    ginfo = info;
    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, startMon, (void*)info);
    pthread_attr_destroy(&attr);

    return 0;
}

int printMsg(struct CbMsg *info)
{
    enum MsgLevel level = ML_INFO;
    switch (info->ocb) {
    case ID_CLOSE:
        level = ML_INFO_FILE;
        break;
    case ID_EXECVE:
        level = ML_INFO_PROC;
        break;
    default:
        break;
    }

    do{
        if(info->path && (info->path[0] == 'p' || info->path[0] == 's'))
            break;
        DMSG(level,
             "info.otid = %llu\tinfo.type = %llu\t"
             "info.gpid = %llu\tinfo.pid = %llu\n",
             info->otid,info->type,info->gpid,info->pid);
        if(info->exe)
            DMSG(level,
                 "info.exe = %s\tinfo.exelen = %llu\n",
                 info->exe,info->exelen);
        if(info->path)
            DMSG(level,
                 "info.path = %s\tinfo.pathlen = %llu\n",
                 info->path,info->pathlen);
    }while(0);

    if(info->exe)  free(info->exe);
    if(info->path) free(info->path);
    if(info) free(info);
    return 0;
}

int main(int argc, char** argv)
{
    PutMsg = printMsg;
    signal(SIGUSR1,SIG_IGN);
    int ret = init();
    if(!ret) createMonThread(atoi(argv[1]));
    sleep(10);
//    pthread_kill(thread_id,SIGUSR1);
    while(1)
    {
        if(ginfo->toexit)
        {
            free(ginfo);
            ginfo = NULL;
            break;
        }
        sleep(1);
    }
    return ret;
}
