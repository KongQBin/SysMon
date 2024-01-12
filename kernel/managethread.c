#include "managethread.h"
static int procInited = 0;
// 此处不要被write误导，该write调用会被追踪进程所解析
// 是利用write调用来与‘追踪进程’进行消息传递,达到控制的目的
// 因为‘追踪进程’利用wait4在进行监控，如果利用信号去打断它的话
// 担心有可能会丢失‘被追踪进程’的事件，进而导致一些难以控制的问题
// 故使用该线程以被追踪的方法来向‘追踪进程’传递控制信息
// 该线程内不要有过多的逻辑，保持逻辑简洁，以保证不会导致‘追踪进程’误判
#define SendMsg(wfd,addr,len)   write(wfd,addr,len);
void* manageThreadFunc(void *val)
{
    InitInfo *info = val;
    info->pid = gettid();
    // 当前线程是被追踪进程进行追踪的
    // 故不要有太多的系统调用逻辑，以免影响追踪进程的性能
    int exitNum = 0;
    int readfd = info->cfd[0];

    int wbuflen;
    int wbufbaselen = sizeof(ManageInfo);
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

int sendManageInfo(ManageInfo *info)
{
    return write(info->tpfd[1],info,sizeof(ManageInfo)) != sizeof(ManageInfo);
}
