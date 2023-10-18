#include "callbacks.h"

int getRegsStrArg(struct pidinfo *info, long arg, char **str, size_t *len)
{
    *str = NULL;
    int ret = 0, run = 1, j = 0;
    size_t size = 0, osize = 0;
    char *strend = NULL;
    while(run)
    {
        if(j == size/WORDLEN)
        {
            osize = size;
            size += PATH_MAX;
            char *tmp = realloc(*str,size);
            if(!tmp) {if(*str) {free(*str); *str = NULL;} ret = -1; break;} // 开辟失败，释放原有，错误退出
            *str = tmp;
            memset(*str+osize,0,PATH_MAX);  // 开辟成功，将新增内存初始化
        }

        for(;!ret && j<size/WORDLEN;++j)
        {
            long tmp = 0;
            tmp = ptrace(PTRACE_PEEKDATA, info->pid, arg + (j*WORDLEN), NULL);
            memcpy(&(*str)[j*WORDLEN], &tmp, WORDLEN);
            strend = memchr(&(*str)[j*WORDLEN],'\0',WORDLEN);
            if(!strend)                               continue; // 未找到结束符号
            if(strend && strend == &((*str)[size-1])) continue; // 找到结束符号但其位于最后一个字节
            // 其它找到结束符的情况
            if(strend)
            {
                *len = strend - *str;       // 根据地址偏移得到字符串长度
                run = 0;
                break;
            }
        }
    }
    return ret;
}

int getRealPath(struct pidinfo *info, char **str, size_t *len)
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
            DMSG(ML_WARN,"realpath err : %s\n",strerror(errno))
        }
    }
#endif

    return ret;
}
