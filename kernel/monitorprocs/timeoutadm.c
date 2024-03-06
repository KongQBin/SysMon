#include "timeoutadm.h"
// 所有时间以秒为单位
typedef struct _SuspendPinfo
{
    pid_t pid;              // 被挂起的pid
    long pasttime;          // 已经被挂起多久了
} SuspendPinfo;

typedef struct _SuspendVector
{
    SuspendPinfo *sinfo;
    long size;              // sinfo的开辟数量
    long number;            // 被挂起的数量（已使用的数量）
    long timeout;           // 超时时间
} SuspendVector;

static const int *writefd = NULL;
SuspendVector gSvector;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()        pthread_mutex_lock(&mutex);
#define UNLOCK()      pthread_mutex_unlock(&mutex);

void *timeoutAdmThread(void *argv)
{
    // 等待这个描述符被初始化完毕
    while(!*writefd)
        usleep(0);
    ManageInfo info;
    while(1)
    {
        memset(&info,0,sizeof(info));
        LOCK();
        if(!gSvector.sinfo) break;
        for(int i=0;i<gSvector.number;++i)
        {
            ++ gSvector.sinfo[i].pasttime;
            // 达到超时时间
            if(gSvector.sinfo[i].pasttime >= gSvector.timeout)
            {
//                DMSG(ML_WARN,"%lu timeout\n",gSvector.sinfo[i].pid);
                info.type = MT_CallPass;
                info.tpid = gSvector.sinfo[i].pid;
                // 通知解除挂起状态
                write(*writefd,&info,sizeof(info));
                delPinfo(gSvector.sinfo[i].pid,i,1);
                // delPinfo会将后面的内存整体往前移动，覆盖当前位置，故--
                --i;
            }
        }
        UNLOCK();
        sleep(0);
    }
    return NULL;
}

int preStopTimeoutAdmThread()
{
    LOCK();
    // 放行全部的挂起
    for(int i=0;i<gSvector.number;++i)
        gSvector.sinfo[i].pasttime = 14;
    UNLOCK();
    return 0;
}

int stopTimeoutAdmThread()
{
    // 等待
    if(gSvector.number)
        return -1;
    LOCK();
    free(gSvector.sinfo);
    memset(&gSvector,0,sizeof(gSvector));
    UNLOCK();
}

int startTimeoutAdmThread(const int *wfd)
{
    writefd = wfd;
    if(!gSvector.sinfo)
    {
        gSvector.sinfo = calloc(1,1024*sizeof(SuspendPinfo));
        if(!gSvector.sinfo)
        {
            DERR(calloc);
            return -1;
        }
        else
        {
            gSvector.size = 1024;       // 总容量
            gSvector.timeout = 0;      // 超时时间
        }
    }

    int ret;
    pthread_t tid;
    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    if(ret)
    {
        DERR(pthread_attr_init);
        return -2;
    }
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&tid, &attr, timeoutAdmThread, NULL);
    if(ret)
    {
        DERR(pthread_create);
        return -3;
    }
    return 0;
}

int addPinfo(const pid_t pid)
{

    LOCK();
    gSvector.sinfo[gSvector.number].pid = pid;
    gSvector.sinfo[gSvector.number].pasttime = 0;
    ++gSvector.number;
    UNLOCK();
    return 0;
}

int delPinfo(const pid_t pid, int index, int locked)
{
    // 如果没有给索引信息，那么自行遍历查找
    if(index == -1)
    {
        for(int i = 0;i<gSvector.number;++i)
            if(gSvector.sinfo[i].pid == pid)
            {
                index = i;
                break;
            }
    }
    if(index == -1)
        return -1;

    if(!locked) LOCK();
    // 让后面的元素覆盖当前的元素
    memmove(gSvector.sinfo+index,gSvector.sinfo+index+1,
            (gSvector.size-index-1)*sizeof(SuspendPinfo));
    // 对于最后一个元素(正常情况下仅仅挂起可执行程序的话，1024被用尽的概率不大)
    if(index == gSvector.size-1)
        memset(gSvector.sinfo+index,0,sizeof(SuspendPinfo));
    --gSvector.number;
    if(!locked) UNLOCK();
    return 0;
}


