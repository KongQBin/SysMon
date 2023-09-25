#pragma once

#define rbSearch(tree,func,id,ret){\
    struct rb_node *node = tree->rb_node;\
    ret = NULL;\
    typeof(ret) tmp = NULL;\
    while (node)\
    {\
        tmp = container_of(node, typeof(*ret), node);\
        if(func(tmp,id,0))\
            node = node->rb_left;\
        else if (func(tmp,id,1))\
            node = node->rb_right;\
        else\
            {ret = tmp;break;}\
    }\
}

#define rbInsert(tree,func,data,ret){\
    struct rb_node **new_node = &(tree->rb_node), *parent = NULL;\
    /* Figure out where to put new_node node */\
    while (*new_node)\
    {\
        typeof(data) this_node = container_of(*new_node, typeof(*data), node);\
        parent = *new_node;\
        if(func(data,this_node,0))\
            new_node = &((*new_node)->rb_left);\
        else if(func(data,this_node,1))\
            new_node = &((*new_node)->rb_right);\
        else\
            {ret = -1;break;}\
    }\
    /* Add new_node node and rebalance tree. */\
    rb_link_node(&data->node, parent, new_node);\
    rb_insert_color(&data->node, tree);\
    ret = 0;\
}

#define rbClear(tree,type,func){\
    while (1)\
    {\
            struct rb_node *node = NULL;\
            node = rb_first(tree);\
            if(!node) break;\
\
            type *call = NULL;\
            call = rb_entry(node, type, node);\
            rb_erase(node, tree);\
\
            if(call) func(call);\
            else printf("call is NULL\n");\
    }\
    free(tree);\
}
