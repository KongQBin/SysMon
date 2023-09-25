#include "pidtree.h"

struct pidinfo *hotSearch(struct rb_root *tree, int id)
{
    if(!tree) return NULL;
    struct pidinfo *data;
    rbSearch(tree,searchCallBack,id,data);
    return data;
}

int hotInsert(struct rb_root *tree, struct pidinfo *data)
{
    if(!tree || !data) return -1;
    int ret = 0;
    rbInsert(tree,insertCallBack,data,ret);
    return ret;
}

void hotClear(struct rb_root *tree)
{
    if(!tree) return;
    rbClear(tree,struct pidinfo,clearCallBack);
}
