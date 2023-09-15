#include <pthread.h>
#include "sysmon.h"

extern pid_t contpid;
extern pthread_t thread_id;
extern struct rb_root *cbTree;

int createMonThread(pid_t pid)
{
     pid_t *ppid = (pid_t*)calloc(1,sizeof(pid_t));
     if(!ppid) return -1;
     *ppid = pid;

    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, startMon, (void*)ppid);
    pthread_attr_destroy(&attr);

    return 0;
}
int main(int argc, char** argv)
{
    signal(SIGUSR1,SIG_IGN);
    int ret = init();
    cbTree = (struct rb_root*)calloc(1,sizeof(struct rb_root));
    if(!cbTree) {return -1;}
    if(!ret) ret = insertCallbackTree(cbTree,ID_WRITE,cbWrite,ceWrite);
    if(!ret) ret = insertCallbackTree(cbTree,ID_FORK,cbFork,ceFork);
    if(!ret) ret = insertCallbackTree(cbTree,ID_CLONE,cbClone,ceClone);
    if(!ret) ret = insertCallbackTree(cbTree,ID_EXECVE,cbExecve,ceExecve);

    if(!ret) createMonThread(atoi(argv[1]));
    sleep(10);
    pthread_kill(thread_id,SIGUSR1);
    while(1) sleep(100);
    return ret;
}
