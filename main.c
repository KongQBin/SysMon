#include <pthread.h>
#include "sysmon.h"

extern pid_t contpid;
extern pthread_t thread_id;

struct ControlInfo *ginfo = NULL;
int createMonThread(pid_t pid)
{
    struct ControlInfo *info = calloc(1,sizeof(struct ControlInfo));
    if(!info) return -1;

    // 设置被监控的ID
    info->tpid = pid;
    // 设置监控到关注的系统调用，在其执行前后的调用函数
    SETFUNC(info,ID_WRITE,cbWrite,ceWrite);
    SETFUNC(info,ID_FORK,cbFork,ceFork);
    SETFUNC(info,ID_CLONE,cbClone,ceClone);
    SETFUNC(info,ID_EXECVE,cbExecve,ceExecve);
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
int main(int argc, char** argv)
{
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
