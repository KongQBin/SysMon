#include "callbacks.h"
#define SYSMON_PATH_MAX 256
int mreadlink(char *originPath, char **targetPath, size_t *len)
{
    int mlen = 0, olen = 0;
    while(1)
    {
        *targetPath = calloc(1,mlen + SYSMON_PATH_MAX);
        if(!*targetPath) return -1;
        olen = mlen + SYSMON_PATH_MAX;
        mlen = readlink(originPath, *targetPath, olen);
        if(mlen < olen) break;
        else free(*targetPath);
    }
    *len = mlen;
    return mlen;
}

int getCwd(const PidInfo *info,char **cwd, size_t *len)
{
    char cwdPath[64] = { 0 };
    sprintf(cwdPath,"/proc/%llu/task/%llu/cwd",info->gpid,info->pid);
    return mreadlink(cwdPath,cwd,len);
}

int getExe(const PidInfo *info,char **exe, size_t *len)
{
    char exePath[64] = { 0 };
    sprintf(exePath,"/proc/%llu/exe",info->gpid);
    return mreadlink(exePath,exe,len);
}

int getFdPath(const PidInfo *info,long fd, char **path, size_t *len)
{
    char fdPath[128] = { 0 };
    sprintf(fdPath,"/proc/%llu/task/%llu/fd/%d",info->gpid,info->pid,fd);
    return mreadlink(fdPath,path,len);
}

int getFdOpenFlag(const PidInfo *info,long fd, int *flags)
{
    int ret = 0;
    char fdInfoPath[128] = { 0 },*tmp = NULL;
    sprintf(fdInfoPath,"/proc/%llu/task/%llu/fdinfo/%lld",info->gpid,info->pid,fd);

    FILE *fp = fopen(fdInfoPath,"r");
    if(!fp)
    {
        if(errno = ENOENT)
        {
            memset(fdInfoPath,0,sizeof(fdInfoPath));
            // 进程目录的fdinfo和线程目录的fdinfo应该是一样的，故替换后再次尝试
            sprintf(fdInfoPath,"/proc/%llu/fdinfo/%lld",info->gpid,fd);
        }
        fp = fopen(fdInfoPath,"r");
        if(!fp)
        {
            // 理论上不应该走到这里，但又经常走到这里，不清楚是我本地bash的bug还是其它原因，
            // 已确认是在close真正陷入内核前调用的，此时fd还没被内核关闭，但对应fd确实不存在

            // 经验证应该属于bug(被追踪的进程确实在关闭一个没被其打开的文件描述符)，
            // 我在本地使用strace工具进行追踪，也会有类似的报错且fd一致，既然属于bug，
            // 那么此处就不再进行打印了，否则打印如果太频繁，会影响性能

            // DMSG(ML_ERR,"fopen fail errcode %d, err is %s : %s\n",
            //      errno,strerror(errno),fdInfoPath);
            return -1;
        }
    }

    for(char buf[512]={0};fgets(buf,sizeof(buf)-1,fp);memset(buf,0,sizeof(buf)))
    {
        if(!strstr(buf,"flags:")) continue;
        tmp = strstr(buf,"\t");
        if(!tmp) {ret = -2; break;}
        ++tmp;
        break;
    }
    if(!ret && tmp)
    {
        char *endptr;
        *flags = strtol(tmp,&endptr,8);
        if(tmp == endptr) ret = -3;
    }
    fclose(fp);
//    DMSG(ML_INFO,"open flags = %d\n",*flags);
    return ret;
}
