#include "callbacks.h"

// 1： 要对哪个进程进行拷贝
// 2： 位于目标进程的起始地址
// 3： 要拷贝到的目标地址
// 4： 要拷贝的长度
// PS： 如果参数4=NULL,则表示要拷贝的是字符串，此时参数3将被自动开辟
int getArg(const pid_t *pid, const long *originaddr, void **targetaddr, size_t *len)
{
//    DMSG(ML_INFO,"len = %lld\n",*len);
    int ret = 0;
    if(*len)    // 有长度，证明有可能是二进制数据
    {
        long readLen = *len;
        for(int i=0;i<*len/WORDLEN+1;++i)
        {
            long tmpbuf = ptrace(PTRACE_PEEKDATA, *pid, *originaddr + (i*WORDLEN), NULL);
            memcpy(&((long*)*targetaddr)[i],&tmpbuf,(readLen > WORDLEN) ? WORDLEN : readLen);
            readLen -= WORDLEN;
        }
    }
    else
    {
        char *tmp = NULL;
        const int baseLen = 32*WORDLEN;
        int reallocLen = baseLen, toBreak = 0;
        for(;!toBreak && (tmp=realloc(tmp,reallocLen));reallocLen*=2)
        {
            // 清空新开辟出的内存
            memset(tmp+(reallocLen!=baseLen?reallocLen/2:0),
                   0,reallocLen!=baseLen?reallocLen/2:reallocLen);
            // 从新开辟出的位置继续进行拷贝
            for(int i=(reallocLen!=baseLen?reallocLen/2/WORDLEN:0); i<reallocLen/WORDLEN; ++i)
            {
                long tmpbuf = ptrace(PTRACE_PEEKDATA, *pid, *originaddr + (i*WORDLEN), NULL);
                memcpy(&((long*)tmp)[i],&tmpbuf,WORDLEN);
                // 查找结束位置
                char *end = memchr(&tmp[i*WORDLEN],'\0',WORDLEN);
                if(end && end!=&(tmp[reallocLen-1]))
                {
                    toBreak = 1;
                    break;
                }
            }
        }
        // 判断以上循环是否出错，无错赋之
        if(tmp)
        {
            *targetaddr = tmp;
            *len = reallocLen;
        }
        else
        {
            DERR(realloc);
            ret = -1;
        }
    }
    return ret;
}

int getRealPath(PidInfo *info, char **str, size_t *len)
{
    int ret = 0;
    char *tmp = NULL;
    // 此处判断是绝对路径还是相对路径，如果是相对路径则转换为绝对路径
    // 注意此处判断是存在缺陷的，如果在文件操作时传入的仅仅是文件名，就无法正常转换了
    // 但是如果以'/'作为条件，那么会将pipe、anon_inode等作为相对路径进行转换
    if(*str[0] == '.')
    {
        size_t cwdlen = 0;
        char *cwd = NULL;
        if(!getCwd(info,&cwd,&cwdlen))
        {

            tmp = realloc(cwd,cwdlen+*len+2);
            if(tmp)
            {
                cwd = tmp;
                memset(cwd+cwdlen,0,*len+2);
                memcpy(cwd+cwdlen,"/",1);
                memcpy(cwd+cwdlen+1,*str,*len);
                free(*str);
                *str = cwd;
            }
            else
                if(tmp) {free(tmp); tmp=NULL;}
        }
        else
        {
            DMSG(ML_ERR,"getCwd exec failure\n");
            ret = -1;
        }
    }

#if 1   /* 如果需要提升效率，可以禁用该段落，不会影响任何处理逻辑 */
    if(strstr(*str,"./"))       // 路径中间包含了 */./* || */../*
    {
        // 此处已经拼接成功
        // 例如变为了：/home/user/../kongbin/sysmon
        // 然后将其转换为全绝对路径形式
        tmp = NULL;
        if((tmp = realpath(*str,tmp)) != NULL)
        {
            free(*str);
            *str = tmp;
            *len = strlen(*str);
        }
        else
        {
            /*
            * 一般是路径长度超过了PATH_MAX，errno = ENAMETOOLONG
            * 无所谓，因为使用例如/home/user/../kongbin/sysmon路径也能满足业务逻辑
            * 转换是因为转换后的/home/kongbin/sysmon更加易于人类阅读，而且以也能适当的减少堆区内存的使用
            */
            DMSG(ML_WARN,"realpath err : %s\n",strerror(errno));
        }
    }
#endif
    return ret;
}
