#include "callbacks.h"
long cbRenameat2(CB_ARGVS)
{
    return 0;
}
long ceRenameat2(CB_ARGVS)
{
    // 初始化参数
    char *cwd = NULL;
    size_t cwdlen = 0;
    int ofd = ARGV_1(argv->cctext->regs);
    int nfd = ARGV_3(argv->cctext->regs);
    int flags = ARGV_5(argv->cctext->regs);
    char *opath = NULL, *npath = NULL;
    size_t opathlen = 0,npathlen = 0;
    if(!getArg(&argv->info->pid,&ARGV_2(argv->cctext->regs),(void**)&opath,&opathlen)
        && !getArg(&argv->info->pid,&ARGV_4(argv->cctext->regs),(void**)&npath,&npathlen))
    {
        if(AT_FDCWD == ofd || AT_FDCWD == nfd) // 相对程序运行路径
            // 获取程序运行路径
            getCwd(argv->info,&cwd,&cwdlen);
        if(AT_FDCWD == ofd)
        {
            char *tmp = realloc(opath,opathlen+cwdlen);
            if(tmp)
            {
                opath = tmp;
                memmove(opath+cwdlen-1,opath,opathlen); // -1是为了抹除 ./ 前面的 .
                memcpy(opath,cwd,cwdlen);
                opathlen += cwdlen;
            }
        }
        if(AT_FDCWD == nfd)
        {
            char *tmp = realloc(npath,npathlen+cwdlen);
            if(tmp)
            {
                npath = tmp;
                memmove(npath+cwdlen-1,npath,npathlen);
                memcpy(npath,cwd,cwdlen);
                npathlen += cwdlen;
            }
        }
//        DMSG(ML_INFO,"ofd=%d, opath=%s, nfd=%d, npath=%s,flags=%d\n",ofd,opath,nfd,npath,flags);
    }
    if(RET(argv->cctext->regs) == 0)
        PutMsg(createMsg(ID_CLOSE, NBLOCK,argv->info->gpid,argv->info->pid,
                         argv->info->exe,argv->info->exelen,
                         opath,opathlen,npath,npathlen));

    if(opath)   free(opath);
    if(npath)   free(npath);
    if(cwd)     free(cwd);
    return 0;
}
