#include "hotree.h"

struct hotfile* hotSearch(struct rb_root *tree, int inode)
{
    struct hotfile *data;
    rbSearch(tree,searchCallBack,inode,data);
    return data;
}

int hotInsert(struct rb_root *tree, struct hotfile *data)
{
    int ret = 0;
    rbInsert(tree,insertCallBack,data,ret);
    return ret;
}

void hotClear(struct rb_root *tree)
{
    rbClear(tree,struct hotfile,clearCallBack);
}
