#include "iteratesys.h"
typedef struct _RealPathInfo
{
    char *realpath;
    size_t pathlen;
} RealPathInfo;
static RealPathInfo gRealPath;

// 按照进(线)程特征再次过滤
static int softwareFilter(const char *procname)
{
    int skip = 0;
    // 初始化gRealPath
    if(gRealPath.pathlen)
        memset(gRealPath.realpath,0,gRealPath.pathlen);
    else
    {
        gRealPath.pathlen = PATH_MAX;
        gRealPath.realpath = calloc(1,gRealPath.pathlen);
        if(!gRealPath.realpath)
        {
            gRealPath.pathlen = 0;
            DERR(calloc);
            skip = 1;
            return skip;
        }
    }

    // 获取/proc目录中软链接exe指向的可执行文件路径
    while(1)
    {
        char softlink[64] = { 0 };
        snprintf(softlink,sizeof(softlink)-1,"/proc/%s/exe",procname);
        if(access(softlink,F_OK))              // 文件不存在
        {
//            DERR(access);
            skip = 1;
            break;
        }

        size_t len = readlink(softlink, gRealPath.realpath, gRealPath.pathlen);
        if(len < 0)                            // 读取失败
        {
            DERR(readlink);
            skip = 1;
            break;
        }
        if(len < gRealPath.pathlen) break;     // 读取成功
        // 到此处证明内存空间不够,扩容
        char *tmp = realloc(gRealPath.realpath,gRealPath.pathlen+PATH_MAX);
        if(tmp)
        {
            gRealPath.realpath = tmp;
            gRealPath.pathlen += PATH_MAX;
            memset(gRealPath.realpath,0,gRealPath.pathlen);
        }
        else
        {
            DERR(realloc);
            skip = 1;
            break;
        }
    }

    do{
        if(skip) break;                          // 以上流程出现问题
        // 内核进程没有exe realpath
        if(!strlen(gRealPath.realpath))
        {
            skip = 1;
            break;
        }
    }while(0);
    return skip;
}

// 过滤进程组，也就是/proc目录下的进程
static int filterGPid(const struct dirent *dir)
{
    int need = 0;
    pid_t gpid = 0;
    char *strend = NULL;
    do
    {
        if(dir->d_type != DT_DIR)                           break;  // 非目录
        if(softwareFilter(dir->d_name))                     break;  // 按照进程特征再次过滤
        gpid = strtoll(dir->d_name,&strend,10);
        if(dir->d_name != strend)
        {
            // 过滤自身
            if(gpid == getpid()) break;
            // 过滤兄弟进程
            int skip = 0;
            for(int j=0;j<gProcNum;++j)
            {
                if(gInitInfo[j].spid == gpid)
                {
                    skip = 1;
                    break;
                }
            }
            if(skip) break;
        }
        need = 1;
    }while(0);
    return need;
}
// 遍历/proc/pid/task/目录,获取所有线程pid
// 传入pid,返回pid数组,数组以0结尾,需自行释放
pid_t *getTask(pid_t gpid)
{
    char taskPath[64] = { 0 };
    sprintf(taskPath,"/proc/%llu/task",gpid);
    DIR *dir = opendir(taskPath);
    if(!dir)
    {
        DERR(opendir);
        return NULL;
    }

    int pidsize = 10;
    pid_t *tpids = calloc(1,pidsize*sizeof(pid_t));
    if(!tpids)
    {
        DERR(calloc);
        return tpids;
    }

    pid_t *index = tpids;
    struct dirent *de;
    while((de = readdir(dir)) != NULL)
    {
        pid_t tpid = 0;
        char *endptr = NULL;
        if (de->d_fileno == 0 || !de->d_name) continue;
        tpid = strtoll(de->d_name,&endptr,10);
        if(de->d_name == endptr || tpid <= 0) continue;    //转换失败||非法的pid
        *index = tpid;
        ++index;
        // 扩容
        if(index == tpids+pidsize-1)
        {
            pid_t *tmp = realloc(tpids,(pidsize*2)*sizeof(pid_t));
            if(tmp)
            {
                memset(tmp+pidsize, 0, pidsize*sizeof(pid_t));
                pidsize *= 2;
                tpids = tmp;
            }
            else
            {
                DERR(realloc);
                break;
            }
        }
    }
    return tpids;
}

int iterateSysThreads(pid_t **pids)
{
    int err = 0;
    FILE *fp = NULL;
    do
    {
        int pidscount = 1024, pidslen = 0;
        *pids = calloc(pidscount,sizeof(pid_t));
        fp = fopen("/tmp/mpids","w+");
        if(!fp || !*pids)
        {
            if(!*pids) DERR(calloc);
            if(!fp) DERR(fopen);
            err = 1;
            break;
        }

        // 遍历proc目录
        struct dirent **namelist;
        const char *directory = "/proc";
        int direntnum = scandir(directory, &namelist, filterGPid, alphasort);
        if (direntnum == -1) {
            DERR(scandir);
            err = 1;
            break;
        }

        char *strend;
        pid_t *tpids,gpid;
        for(int i = 0; i < direntnum; ++i)
        {
            tpids = NULL;
            strend = NULL;
            gpid = strtoll(namelist[i]->d_name,&strend,10);
            if(gpid && strend != namelist[i]->d_name)
            {
                fprintf(fp," %d\n",gpid);
                tpids = getTask(gpid);
                if(!tpids) continue;
                pid_t *ttmp = tpids;
                while(*ttmp)
                {
                    if(*(ttmp+1))
                        fprintf(fp,"\t├---- %d\n",*ttmp);
                    else
                        fprintf(fp,"\t└---- %d\n",*ttmp);
                    (*pids)[pidslen] = *ttmp;
                    ++ pidslen;

                    // 扩容
                    if(pidslen >= pidscount-1)
                    {
                        pid_t *tmpids = realloc(*pids,(pidscount+pidscount) * sizeof(pid_t));
                        if(!tmpids)
                        {
                            DERR(realloc);
                            err = 1;
                            break;  // skip while
                        }
                        // 初始化新内存区域
                        memset(tmpids+pidscount,0,pidscount*sizeof(pid_t));
                        // 移交内存所有权
                        pidscount += pidscount;
                        *pids = tmpids;
                    }
                    ++ttmp;
                }
                free(tpids);
            }
            if(err) break;  // skip for
        }
        free(namelist);
    }while(0);

    if(err && *pids)
    {
        free(*pids);
        *pids = NULL;
    }
    if(fp) fclose(fp);
    return err;
}
