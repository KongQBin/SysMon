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

int getCwd(PidInfo *info,char **cwd, size_t *len)
{
    char cwdPath[64] = { 0 };
    sprintf(cwdPath,"/proc/%llu/task/%llu/cwd",info->gpid,info->pid);
    return mreadlink(cwdPath,cwd,len);
}

int getExe(PidInfo *info,char **exe, size_t *len)
{
    char exePath[64] = { 0 };
    sprintf(exePath,"/proc/%llu/exe",info->gpid);
    return mreadlink(exePath,exe,len);
}

int getFdPath(PidInfo *info,long fd, char **path, size_t *len)
{
    char fdPath[128] = { 0 };
    sprintf(fdPath,"/proc/%llu/task/%llu/fd/%d",info->gpid,info->pid,fd);
    return mreadlink(fdPath,path,len);
}

int getFdOpenFlag(PidInfo *info,long fd, int *flag)
{
    int ret = 0;
    char fdInfoPath[128] = { 0 },*tmp = NULL;
    sprintf(fdInfoPath,"/proc/%llu/task/%llu/fdinfo/%lld",info->gpid,info->pid,fd);

    FILE *fp = fopen(fdInfoPath,"r");
    if(!fp)
    {
        DERR(fopen);
        return -1;
    }

    char buf[512] = { 0 };
    while(fgets(buf,sizeof(buf),fp))
    {
        if(!strstr(buf,"flags:")) continue;
        tmp = strstr(buf,"\t");
        if(!tmp) {ret = -2; break;}
        ++tmp;
        break;
    }
    if(!ret)
    {
        char *endptr;
        *flag = strtol(tmp,&endptr,8);
        if(tmp == endptr) ret = -3;
    }
    fclose(fp);
//    DMSG(ML_INFO,"open flag = %d\n",*flag);
    return ret;
}
