#include "sysmon.h"

int gmaintoexit = 0;
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
//        DMSG(level,
//             "info.otid = %llu\tinfo.type = %llu\t"
//             "info.gpid = %llu\tinfo.pid = %llu\n",
//             info->otid,info->type,info->gpid,info->pid);
        if(info->exe)
            DMSG(level,
                 "info.exelen  = %llu\tinfo.exe = %s\n",
                 info->exelen,info->exe);
        if(info->path)
        {
            if(info->path[0] != '/') break;
            DMSG(level,
                 "info.pathlen = %llu\tinfo.path = %s\n",
                 info->pathlen,info->path);
        }
    }while(0);

    if(info->exe)  free(info->exe);
    if(info->path) free(info->path);
    if(info) free(info);
    return 0;
}

void sigOptions(int sig)
{
    printf(" sig = %d\n",sig);
    if(sig == SIGINT || sig == SIGTERM)
    {
        DMSG(ML_WARN,"\n正在通知退出，请稍候...\n");
        gmaintoexit = 1;
    }
    return;
}

int main(int argc, char** argv)
{
    signal(SIGINT,sigOptions);  // Ctrl + c
    signal(SIGTERM,sigOptions); // kill -15
    StartSystemMonitor();
    return 0;
}
