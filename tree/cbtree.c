#include "cbtree.h"

//struct rb_root g_cbTree = RB_ROOT;
//struct rb_root g_cbTree = {NULL,};

struct syscall *cbSearch(struct rb_root *tree,int id)
{
    struct syscall *data;
    rbSearch(tree,searchCallBack,id,data);
    return data;
}


int cbInsert(struct rb_root *tree,struct syscall *data)
{
    int ret = 0;
    rbInsert(tree,insertCallBack,data,ret);
    return ret;
}

void cbClear(struct rb_root *tree)
{
    rbClear(tree,struct syscall,clearCallBack);
}
////使free立即将内存返还给系统
//#include <malloc.h>
//int ret = mallopt(M_MXFAST, 0);
//if (1 == ret) printf("mallopt set M_MXFAST = %d succeed\n", 0);
//else printf("mallopt set M_MXFAST = %d failed, errno: %d, desc: %s\n", 0, errno, strerror(errno));
