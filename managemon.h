#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hotinfo.h"
#include "cbstruct.h"

static inline CbMsg* createMsg(int ocb,ETYPE type,int wfd,pid_t gpid,pid_t pid,const char *exe,int64_t exelen,
                                      char *path,int64_t pathlen,char *npath,int64_t npathlen)
{
    CbMsg *msg = calloc(1,sizeof(CbMsg));
    if(msg)
    {
        msg->wfd = wfd;
        msg->ocb = ocb;
        msg->type = type;
        msg->otid = gettid();
        msg->gpid = gpid;
        msg->pid = pid;
        msg->exe = exe;
        msg->exelen = exelen;
        msg->opath = path;
        msg->opathlen = pathlen;
        msg->tpath = npath;
        msg->tpathlen = npathlen;
    }
    return msg;
}

// 谨记:传入该回调的结构体以及其中的指针若未保存至上下文则必须自行释放
int (*PutMsg)(CbMsg *info);
