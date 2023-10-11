#include "pidtree.h"

struct pidinfo *pidSearch(struct rb_root *tree, pid_t id)
{
    if(!tree) return NULL;
    struct pidinfo *data;
    rbSearch(tree,searchPidInfoCallBack,id,data);
    return data;
}

int pidDelete(struct rb_root *tree, pid_t id)
{
    if(!tree) return -1;
    struct pidinfo *info = pidSearch(tree,id);
    if(!info) return -2;
    rb_erase(&info->node, tree);
    free(info);
    info = NULL;
    return 0;
}

int pidInsert(struct rb_root *tree, struct pidinfo *data)
{
    if(!tree || !data) return -1;

    // 检查若存就认为另一个相同pid的进程在上次退出时没被删除
    struct pidinfo *tmp = pidSearch(tree,data->pid);
    if(tmp) pidDelete(tree,data->pid);  // 直接删掉

    int ret = 0;
    DMSG(ML_INFO,"Insert pid tree data->pid = %d\n",data->pid);
    rbInsert(tree,insertPidInfoCallBack,data,ret);
    return ret;
}

void pidClear(struct rb_root *tree)
{
    if(!tree) return;
    rbClear(tree,struct pidinfo,clearPidInfoCallBack);
}

struct pidinfo *createPidInfo(pid_t pid, pid_t gpid, pid_t ppid)
{
    struct pidinfo *info = calloc(1,sizeof(struct pidinfo));
    if(info)
    {
        info->pid = pid;
        info->gpid = gpid;
        info->ppid = ppid;
    }
    return info;
}

int64_t pidTreeSize(struct rb_root *tree)
{
    int64_t size = 0;
    while (1)
    {
        struct rb_node *node = NULL;
        node = rb_first(tree);
        if(!node) break;
        ++node;
    }
    return size;
}
