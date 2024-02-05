#include "pidtree.h"
#include <errno.h>
#include <string.h>

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

PidInfo *createPidInfo(pid_t pid, pid_t gpid, pid_t ppid)
{
    PidInfo *info = calloc(1,sizeof(PidInfo));
    if(info)
    {
        info->pid = pid;
        info->gpid = gpid;
        info->ppid = ppid;
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
