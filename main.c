#include "sysmon.h"
#include "msgloop.h"

int globalexit = 0;
int printMsg(struct CbMsg *info)
{
    if(!info) return -1;
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
        if(info->opath && (info->opath[0] == 'p' || info->opath[0] == 's'))
            break;
//        DMSG(level,
//             "info.otid = %llu\tinfo.type = %llu\t"
//             "info.gpid = %llu\tinfo.pid = %llu\n",
//             info->otid,info->type,info->gpid,info->pid);
        if(info->opath[0] != '/') break;
        if(!info->tpath)
            DMSG(level,"Target exe %s operates on file %s\n",
                 info->exe,info->opath);
        else
            DMSG(level,"Target exe %s operates from file %s to %s\n",
                 info->exe,info->opath,info->tpath);
    }while(0);

//    if(info->exe)
//    {
//        free(info->exe);
//        info->exe = NULL;
//    }
    if(info->opath) free(info->opath);
    if(info) free(info);
    return 0;
}

//extern int gProcNum;
//extern InitInfo gInitInfo[PROC_MAX];
static void sigOptions(int sig)
{
    printf(" signal %d\n",sig);
    if(sig == SIGINT || sig == SIGTERM)
    {
        DMSG(ML_WARN,"\n正在通知退出，请稍候...\n");
//        // 遍历并通知各个子进程退出
//        ManageInfo info;
//        for(int i=0;i<gProcNum;++i)
//        {
//            memcpy(info.tpfd,gInitInfo[i].cfd,sizeof(info.tpfd));
//            info.type = MT_ToExit;
//            if(sendManageInfo(&info))
//                DERR(sendManageInfo);
//        }
    }
}

// 标准输出重定向
void redirectStdout()
{
    char *filename = "output.log";
    FILE *fp = freopen(filename,"w+",stdout);       //标准输出重定向到文件output.log
    if(fp)
        DMSG(ML_INFO,"Redirect stdout to %s\n", filename);
    else
        DERR(freopen);
}

int main(int argc, char** argv)
{
//    redirectStdout();
    // 设置进程优先级 -20 是最高优先级(子进程会继承该优先级)
    setpriority(PRIO_PROCESS, getpid(), -20);
    // 启动监控进程
    if(StartSystemMonitor()) return -1;
    // 给主进程单独设置信号处理函数
    signal(SIGINT,sigOptions);  // Ctrl + c
    signal(SIGTERM,sigOptions); // kill -15
    // 进入主进程消息循环
    return MainMessageLoop();
}

////使free立即将内存返还给系统
//#include <malloc.h>
//int ret = mallopt(M_MXFAST, 0);
//if (1 == ret) printf("mallopt set M_MXFAST = %d succeed\n", 0);
//else printf("mallopt set M_MXFAST = %d failed, errno: %d, desc: %s\n", 0, errno, strerror(errno));
