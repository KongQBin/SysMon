#include "pidtree.h"
#include <errno.h>
#include <string.h>
#include <dirent.h>

PidInfo *pidSearch(struct rb_root *tree, pid_t id)
{
//    DMSG(ML_INFO,"pidSearch pid tree id = %d\n",id);
    if(!tree) return NULL;
    PidInfo *data = NULL;
//    rbSearch(tree,searchPidInfoCallBack,id,data);


    struct rb_node *mnode = tree->rb_node;
    data = NULL;
    PidInfo * tmp = NULL;
    while (mnode)
    {
        tmp = container_of(mnode, typeof(*data), node);
//        printf("mnode = %x\toffsetof(struct pidinfo,node) = %x\n",mnode,offsetof(struct pidinfo,node));
//        printf("tmp = %x %d\tmnode.pid = %d\n",mnode-offsetof(struct pidinfo,node),mnode-offsetof(struct pidinfo,node),tmp->pid);
        if(tmp->pid > id)
            mnode = mnode->rb_left;
        else if (tmp->pid < id)
            mnode = mnode->rb_right;
        else
        {
            data = tmp;
            break;
        }
    }
    return data;
}

int pidDelete(struct rb_root *tree, pid_t id)
{
//    DMSG(ML_ERR,"pidDelete\n");
//    printf("pidDelete\n");
    if(!tree) return -1;

    PidInfo *info = pidSearch(tree,id);
    if(!info) return -2;

    rb_erase(&info->node, tree);

    if(info->exe) free((char*)info->exe);

    clearContextArgvs(&info->cctext);

    free(info);
    info = NULL;
//    DMSG(ML_INFO,"Delete pid tree data->pid = %d\n",id);
    return 0;
}

int pidInsert(struct rb_root *tree, PidInfo *data)
{
    if(!tree || !data) return -1;

    // 检查若存就认为另一个相同pid的进程在上次退出时没被删除
    PidInfo *tmp = pidSearch(tree,data->pid);
    if(tmp) pidDelete(tree,data->pid);  // 直接删掉,后面重新插入

    int ret = 0;
//    DMSG(ML_INFO,"Insert pid tree data->pid = %d\n",data->pid);
    rbInsert(tree,insertPidInfoCallBack,data,ret);

    struct rb_node **new_node = &(tree->rb_node), *parent = NULL;
    /* Figure out where to put new_node node */
    while (*new_node)
    {
        typeof(data) this_node = container_of(*new_node, PidInfo, node);
        parent = *new_node;
        if(data->pid < this_node->pid)
            new_node = &((*new_node)->rb_left);
        else if(data->pid > this_node->pid)
            new_node = &((*new_node)->rb_right);
        else
        {ret = -1;break;}
    }
    if(ret != -1)
    {
        /* Add new_node node and rebalance tree. */
//        DMSG(ML_INFO,"Adata.pid = %d\n",data->pid);
        rb_link_node(&data->node, parent, new_node);
//        DMSG(ML_INFO,"Bdata.pid = %d\n",data->pid);
        rb_insert_color(&data->node, tree);
//        DMSG(ML_INFO,"Cdata.pid = %d\n",data->pid);
    }
//    ret = 0;


    return ret;
}

void pidClear(struct rb_root *tree)
{
    if(!tree) return;
    rbClear(tree,PidInfo,clearPidInfoCallBack);
}
extern int getExe(PidInfo *info,char **exe, size_t *len);
static int getPidInfoFromProc(PidInfo *pinfo)
{
    int ret = 0;
    char pidPath[64] = { 0 };
    // 获取进程组的id
    if(!pinfo->gpid)
    {
        DIR *dir = opendir("/proc");
        if(!dir) {DMSG(ML_WARN,"opendir : %s\n",strerror(errno)); return -1;}
        struct dirent *entry;
        while(entry = readdir(dir))
        {
            if(entry->d_name[0] == '.') continue;
            if(DT_DIR == entry->d_type)
            {
                sprintf(pidPath,"/proc/%s/task/%llu",entry->d_name,pinfo->pid);
                if(!access(pidPath,F_OK))
                {
                    char *tmp = pidPath + strlen("/proc/");
                    char *tmpend = strstr(tmp,"/");
                    if(tmpend) tmpend[0] = '\0';
                    else {ret = -2; break;}

                    char *strend;
                    pinfo->gpid = strtoll(tmp,&strend,10);
                    if(strend == tmp) {pinfo->gpid = 0; ret = -3;}
                    else ret = 0;
                    tmpend[0] = '/';
                    break;
                }
            }
        }
        closedir(dir);
    }
    else
        sprintf(pidPath,"/proc/%llu/task/%llu",pinfo->gpid,pinfo->pid);

    // 获取可执行程序路径
    if(!ret && !pinfo->exe && getExe(pinfo,(char**)&pinfo->exe,&pinfo->exelen) <= 0)
        ret = -4;

    // 获取父进程的id
    if(!ret && !pinfo->ppid)
    {
        // 该过程并不修改ret的值
        // 因为当前ppid可有可无
        strcat(pidPath,"/status");
        FILE *fp = fopen(pidPath,"r");
        if(fp)
        {
            char buf[128] = {0};
            while(fgets(buf,sizeof(buf)-1,fp))
            {
                char *tmp = strstr(buf,"PPid:");
                if(!tmp) continue;
                tmp += strlen("PPid:");
                while(++tmp[0] == ' ');
                char *strend;
                pinfo->ppid = strtoll(tmp,&strend,10);
                if(strend == tmp) {pinfo->ppid = 0;}
                break;
            }
            fclose(fp);
        }
    }
    return ret;
}

PidInfo *createPidInfo(pid_t pid, pid_t gpid, pid_t ppid)
{
    PidInfo *info = calloc(1,sizeof(PidInfo));
    if(info)
    {
        info->pid = pid;
        // gpid和ppid在此处可能为0
        info->gpid = gpid;
        info->ppid = ppid;
        // 通过访问/proc/目录，获取该进程的一些详细信息
        if(getPidInfoFromProc(info))
            DMSG(ML_ERR,"%llu getRelationalPid fail\n",pid);
    }
    else
    {
        DMSG(ML_ERR,"calloc err is %s\n",strerror(errno));
    }
    return info;
}

PidInfo* getStruct(struct rb_node* node)
{
    return container_of(node, PidInfo, node);
}

int64_t pidTreeSize(struct rb_root *tree)
{
    int64_t size = 0;
    for(struct rb_node *node = rb_first(tree); node; node = rb_next(node)) ++size;
    return size;
}
