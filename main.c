#include <pthread.h>
#include "sysmon.h"
#include <sys/sysctl.h>
#include <dirent.h>

struct ControlInfo **ginfo = NULL;
unsigned long ginfolen = 0;
int gmaintoexit = 0;

pid_t apids[256] = {0};
int apidslen = 0;
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
    info->ptree.rb_node = NULL;
    info->ftree.rb_node = NULL;
    // 设置阻塞模式
//    SETBLOCK(info,ID_EXECVE);

    struct ControlInfo **tmp = realloc(ginfo,(ginfolen+1)*sizeof(struct ControlInfo *));
    if(!tmp) { if(info) free(info); return -1;}
    ginfo = tmp;
    ginfo[ginfolen] = info;
    ++ginfolen;

    pthread_t thread_id;
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
//        DMSG(level,
//             "info.otid = %llu\tinfo.type = %llu\t"
//             "info.gpid = %llu\tinfo.pid = %llu\n",
//             info->otid,info->type,info->gpid,info->pid);
        if(info->exe)
            DMSG(level,
                 "info.exelen  = %llu\tinfo.exe = %s\n",
                 info->exelen,info->exe);
        if(info->path)
            DMSG(level,
                 "info.pathlen = %llu\tinfo.path = %s\n",
                 info->pathlen,info->path);
    }while(0);

    if(info->exe)  free(info->exe);
    if(info->path) free(info->path);
    if(info) free(info);
    return 0;
}

int filterGPid(const struct dirent *dir)
{
    if(dir->d_type != DT_DIR)   return 0;       // 不是目录

    char *procPath = "/proc";
    char statusPath[64] = { 0 };
    snprintf(statusPath,sizeof(statusPath)-1,"%s/%s/status",procPath,dir->d_name);
    if(access(statusPath,F_OK)) return 0;       // 目录中没有status文件

    FILE *fp = fopen(statusPath,"r");
    if(!fp)         return 0;                   //文件打开失败
    char tmp[256] = {0};
    int spik = 0;
    while(fgets(tmp,sizeof(tmp)-1,fp))
    {
        char *tmpp = strstr(tmp,"PPid:");
        if(!tmpp)    continue;
        tmpp += strlen("PPid:");
        while(++tmpp[0] == ' ');

        pid_t ppid = 0;
        char *strend;
        ppid = strtoll(tmpp,&strend,10);
        if(strend == tmpp)      spik = 1;     // 转换失败
        if(ppid == 2)           spik = 1;     // 父进程等于2
        break;
    }
    fclose(fp);
    if(spik) return 0;

    char *strend;
    pid_t gpid = strtoll(dir->d_name,&strend,10);
    if(dir->d_name != strend
        && (gpid == getpid() || gpid == 2)) return 0;   //转换成功且（pid等于自身或等于kthread）
    return 1;
}
int startSysMon()
{
    const char *directory = "/proc";
    struct dirent **namelist;
    int num_entries;

    num_entries = scandir(directory, &namelist, filterGPid, alphasort);
    if (num_entries == -1) {
        perror("scandir");
        return 1;
    }

    for (int i = 0; i < num_entries; ++i) {
//        printf("%s\n", namelist[i]->d_name);
        pid_t gpid;
        char *strend;
        gpid = strtoll(namelist[i]->d_name,&strend,10);
        if(strend != namelist[i]->d_name)
            if(i < 2) createMonThread(gpid);;
        free(namelist[i]);
    }
    free(namelist);
    return 0;
}

void sigOptions(int sig)
{
    printf(" sig = %d\n",sig);
    if(sig == SIGINT || sig == SIGTERM)
    {
        DMSG(ML_WARN,"\n正在通知退出，请稍候...\n")
        for(int i=0;i<ginfolen;++i)
        {
            ginfo[i]->toexit = 1;       // 使所有线程都退出
        }
    }
    return;
}

int main(int argc, char** argv)
{
    PutMsg = printMsg;
    signal(SIGINT,sigOptions);  // Ctrl + c
    signal(SIGTERM,sigOptions); // kill -15
    int ret = init();


//    createMonThread(atoi(argv[1]));
//    sleep(2);
//    createMonThread(atoi(argv[2]));

    startSysMon();

    sleep(10);
    while(!gmaintoexit)
    {
        int runing = 0;
        for(int i=0;!runing && i<ginfolen;++i)
        {
            if(!ginfo[i]->exit)
            {
                runing = 1; // 存在正在工作的线程
                break;
            }
//            else
//            {
//                // ginfo[i] 线程已退出

//                free(ginfo[i]);
//                if(i == ginfolen-1)
//                {
//                    struct ControlInfo *tmp = realloc();
//                }
//                memcpy(ginfo[i],ginfo[i])

//            }
        }

        if(!runing) // 没有正在运行的子线程了，那么就去释放
        {
            for(int i=0;i<ginfolen;++i)
            {
                free(ginfo[i]);
                ginfo[i] = NULL;
            }
            free(ginfo);
            ginfo = NULL;
            gmaintoexit = 1;
            break;
        }
        sleep(1);
    }
    return ret;
}
