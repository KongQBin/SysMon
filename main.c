#include <pthread.h>
#include "sysmon.h"
#include <sys/sysctl.h>
#include <dirent.h>

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

int mreadlinkFilter(char *originPath)
{
    int ret = 0;
    size_t mlen = 0, olen = 0;
    char *targetPath = NULL;
    while(1)
    {
        targetPath = calloc(1,mlen + 256);
        if(!targetPath) {ret = 1; break;}
        olen = mlen + 256;
        mlen = readlink(originPath, targetPath, olen);
        if(!targetPath) {ret = 1; break;}
        if(mlen < olen) break;
        else {free(targetPath);targetPath = NULL;}
    }
    if(!ret && targetPath)
    {
        // 内核进程没有exe
        if(!strlen(targetPath))
            ret = 1;
        // Xorg但凡使用图形化就会刷新，也不监控
        char *index = NULL;
        if((index = strstr(targetPath,"Xorg")) != NULL)
        {
            if((index + strlen("Xorg"))[0] == '\0')
                ret = 1;
        }
    }
    if(targetPath) free(targetPath);
    return ret;
}

// 过滤进程组
int filterGPid(const struct dirent *dir)
{
    int ret = 0;
    pid_t gpid = 0;
    char *strend = NULL;
    char statusPath[64] = { 0 };
    do
    {
        if(dir->d_type != DT_DIR)                           break;  // 非目录
        snprintf(statusPath,sizeof(statusPath)-1,"/proc/%s/exe",dir->d_name);
        if(access(statusPath,F_OK))                         break;  // 目录中不包含exe文件
        if(mreadlinkFilter(statusPath))                     break;  // 进一步过滤
        gpid = strtoll(dir->d_name,&strend,10);
        if(dir->d_name != strend && (gpid == getpid()))     break;  // 转换成功但pid等于自身
        ret = 1;
    }while(0);
    return ret;
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
    PutMsg = printMsg;

    signal(SIGINT,sigOptions);  // Ctrl + c
    signal(SIGTERM,sigOptions); // kill -15

    int ret = init();
    ControlInfo *info = calloc(1,sizeof(ControlInfo));
    if(!info)
    {
        DMSG(ML_ERR, "Create ControlInfo err : %s\n",strerror(errno));
        return -1;
    }
    // 设置监控到关注的系统调用，在其执行前后的调用函数
    //    SETFUNC(info,ID_WRITE,cbWrite,ceWrite);
    //    SETFUNC(info,ID_FORK,cbFork,ceFork);
    //    SETFUNC(info,ID_CLONE,cbClone,ceClone);
    SETFUNC(info,ID_EXECVE,cbExecve,ceExecve);
    SETFUNC(info,ID_CLOSE,cbClose,ceClose);
    //    SETFUNC(info,ID_OPENAT,cbOpenat,ceOpenat);
    info->ptree.rb_node = NULL;
    info->ftree.rb_node = NULL;
    // 设置阻塞模式
    //    SETBLOCK(info,ID_EXECVE);
    startSysmon(info);

    while(!gmaintoexit) sleep(1);
    return ret;
}
