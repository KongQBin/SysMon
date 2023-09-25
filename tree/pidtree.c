#include "pidtree.h"

struct pidinfo *pidSearch(struct rb_root *tree, pid_t id)
{
    if(!tree) return NULL;
    struct pidinfo *data;
    rbSearch(tree,searchCallBack,id,data);
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
    int ret = 0;
    rbInsert(tree,insertCallBack,data,ret);
    return ret;
}

void pidClear(struct rb_root *tree)
{
    if(!tree) return;
    rbClear(tree,struct pidinfo,clearCallBack);
}
