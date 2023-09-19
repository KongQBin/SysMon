#include "cbtree.h"

//struct rb_root g_cbTree = RB_ROOT;
//struct rb_root g_cbTree = {NULL,};

struct syscall *cbSearch(struct rb_root *tree,int id)
{
    struct syscall *data;
    rbSearch(tree,searchCallBack,id,data);
    return data;
//    struct rb_node *node = tree->rb_node;
//    while (node)
//    {
//        struct syscall *data = container_of(node, struct syscall, node);
//        if (data->id > id)
//            node = node->rb_left;
//        else if (data->id < id)
//            node = node->rb_right;
//        else
//            return data;
//    }
//    return NULL;
}


int cbInsert(struct rb_root *tree,struct syscall *data)
{
    int ret = 0;
    rbInsert(tree,insertCallBack,data,ret);
    return ret;
//    struct rb_node **new_node = &(tree->rb_node), *parent = NULL;
//    /* Figure out where to put new_node node */
//    while (*new_node)
//    {
//        struct syscall *this_node = container_of(*new_node, struct syscall, node);
//        parent = *new_node;
//        if (data->id < this_node->id)
//            new_node = &((*new_node)->rb_left);
//        else if (data->id > this_node->id)
//            new_node = &((*new_node)->rb_right);
//        else
//            return -1;
//    }
//    /* Add new_node node and rebalance tree. */
//    rb_link_node(&data->node, parent, new_node);
//    rb_insert_color(&data->node, tree);
//    return 0;
}


void cbClear(struct rb_root *tree)
{
    ////使free立即将内存返还给系统
    //#include <malloc.h>
    //int ret = mallopt(M_MXFAST, 0);
    //if (1 == ret) printf("mallopt set M_MXFAST = %d succeed\n", 0);
    //else printf("mallopt set M_MXFAST = %d failed, errno: %d, desc: %s\n", 0, errno, strerror(errno));
    rbClear(tree,struct syscall,clearCallBack);
//    while (1)
//    {
//        struct rb_node *node = NULL;
//        node = rb_first(tree);
//        if(!node) break;

//        struct syscall *call = NULL;
//        call = rb_entry(node, struct syscall, node);
//        rb_erase(node, tree);

//        if(call) free(call);
//        else printf("call is NULL\n");
//    }
}

//    // delete
//    struct syscall *data = mysearch(&root, "walrus");
//    if (data)
//    {
//        rb_erase(&data->node, &root);
//        myfree(data);
//    }

//    // for
//    struct rb_node *node;
//    for (struct rb_node *node = rb_first(&root); node; node = rb_next(node))
//        printf("key=%s\n", rb_entry(node, struct syscall, node)->func);
