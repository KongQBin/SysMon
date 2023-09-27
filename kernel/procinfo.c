#include "procinfo.h"

//                  目标地址            原大小            目标大小
int resizeMem(void *targetPtr, size_t originSize, size_t targetSize)
{
    // 调整内存大小
    void *tmp = realloc(targetPtr, targetSize);
    if(!tmp) return -1;
    targetPtr = tmp;
    // 清空新内存区域
    if(originSize < targetSize)
        memset(targetPtr + originSize, 0,  targetSize - originSize);
    return 0;
}

int initThreadInfo(struct ThreadInfo *info)
{
    if(info) return 1;
    // 创建ThreadInfo
    info = calloc(1,sizeof(struct ThreadInfo));
    if(!info) return -1;
    memset(info,0,sizeof(struct ThreadInfo));

    // 创建ProcInfo
    resizeMem(info->pInfo,0,ONE_PROC_INFO_SIZE*sizeof(struct ProcInfo));
    if(!info->pInfo) {free(info);return -2;}
    info->pInfoSize = ONE_PROC_INFO_SIZE;

    // 为ProcInfo[0]创建pthread_t
    resizeMem(info->pInfo[0]->tid,0,ONE_PTHREAD_SIZE*sizeof(pthread_t));
    if(!info->pInfo[0]->tid) {free(info->pInfo);free(info);return -3;}
    info->pInfo[0]->tidSize = ONE_PTHREAD_SIZE;

    return 0;
}

int addTid(struct ProcInfo *pInfo, pthread_t tid)
{
    if(pInfo->tidLen == pInfo->tidSize)
    {
        if(!resizeMem(pInfo->tid,pInfo->tidLen*sizeof(pthread_t),
                       pInfo->tidLen*sizeof(pthread_t)+ONE_PTHREAD_SIZE*sizeof(pthread_t)))
            pInfo->tidSize += ONE_PTHREAD_SIZE;
        else
            return -1;
    }
    pInfo->tid[pInfo->tidLen] = tid;
    ++ pInfo->tidLen;
    return 0;
}

struct ProcInfo *creatProcInfo(pid_t pid)
{
    struct ProcInfo *pInfo = NULL;
    if(!resizeMem(pInfo,0,sizeof(struct ProcInfo)))
    {
        if(!resizeMem(pInfo->tid,0,ONE_PTHREAD_SIZE*sizeof(pthread_t)))
        {
            pInfo->pid = pid;       //tgid
            pInfo->tid[0] = pid;    //pid
            ++ pInfo->tidLen;
            pInfo->tidSize = ONE_PTHREAD_SIZE;
            return pInfo;
        }
        else
            free(pInfo);
    }
    return NULL;
}

void delProcInfo(struct ProcInfo *pInfo)
{
    if(pInfo)
    {
        if(pInfo->tid)
            free(pInfo->tid);
        free(pInfo);
        pInfo->tid = NULL;
        pInfo = NULL;
    }
}

int addProcInfo(struct ThreadInfo *info, struct ProcInfo *pInfo)
{
    if(info->pInfoLen == info->pInfoSize)
    {
        if(!resizeMem(info->pInfo,info->pInfoSize*sizeof(struct ProcInfo),
                      info->pInfoSize*sizeof(struct ProcInfo)+ONE_PROC_INFO_SIZE*sizeof(struct ProcInfo)))
        {
            info->pInfoSize += ONE_PROC_INFO_SIZE;
        }
        else
        {
            dmsg("realloc return is NULL, err is %s\n",strerror(errno));
            return -1;
        }
    }
    memcpy(info->pInfo + info->pInfoLen * sizeof(struct ProcInfo), pInfo, sizeof(struct ProcInfo));
    ++ info->pInfoLen;
    delProcInfo(pInfo);
    return 0;
}

int addPid(struct ThreadInfo *pInfo, pid_t *pid)
{
    if(pInfo->pidLen == pInfo->pidSize)
    {
        // 内存扩容
        pid_t *tmp = (pid_t*)realloc(pInfo->pids,pInfo->pidSize + 10);
        if(tmp)
        {
            pInfo->pids = tmp;
            pInfo->pidSize += 10;
            for(int i=pInfo->pidLen;i<pInfo->pidSize;++i)
                pInfo->pids[i] = 0;
        }
        else
        {
            dmsg("realloc return is NULL, err is %s\n",strerror(errno));
            return -1;
        }
    }
    pInfo->pids[pInfo->pidLen] = *pid;
    ++ pInfo->pidLen;
    return 0;
}

int delPid(struct ThreadInfo *pInfo, pid_t *pid)
{
    int deled = 0;
    for(int i=0;i<pInfo->pidLen;++i)
    {
        if(!deled)
        {
            if(pInfo->pids[i] == *pid) {pInfo->pids[i] = 0; deled = 1;}
        }
        else
            // 把后面的元素向前移动（节省后续遍历时间）
            pInfo->pids[i-1] = pInfo->pids[i];
    }
    if(deled) --pInfo->pidLen;

    if(pInfo->pidSize - pInfo->pidLen > 10)
    {
        // 减小可用空间，并未释放（被减小的空间供下次m/c/realloc使用）
        pid_t *tmp = (pid_t*)realloc(pInfo->pids,pInfo->pidSize - 10);
        if(tmp) pInfo->pids = tmp;
    }
    return 0;
}
