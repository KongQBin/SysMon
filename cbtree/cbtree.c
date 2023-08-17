#include "cbtree.h"

struct rb_root g_cbTree = RB_ROOT;
struct syscall *cbSearch(int id)
{
    struct rb_node *node = g_cbTree.rb_node;
    while (node)
    {
        struct syscall *data = container_of(node, struct syscall, node);
        if (data->id < id)
            node = node->rb_left;
        else if (data->id > id)
            node = node->rb_right;
        else
            return data;
    }
    return NULL;
}

int cbInsert(struct syscall *data)
{
    struct rb_node **new_node = &(g_cbTree.rb_node), *parent = NULL;
    /* Figure out where to put new_node node */
    while (*new_node)
    {
        struct syscall *this_node = container_of(*new_node, struct syscall, node);
        parent = *new_node;
        if (data->id < this_node->id)
            new_node = &((*new_node)->rb_left);
        else if (data->id > this_node->id)
            new_node = &((*new_node)->rb_right);
        else
            return -1;
    }
    /* Add new_node node and rebalance tree. */
    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, &g_cbTree);
    return 0;
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
