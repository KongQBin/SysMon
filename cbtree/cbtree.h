#pragma once
#include "rbtree.h"

extern struct rb_root g_cbTree;
struct syscall
{
    struct rb_node node;
    int   id;           // 系统调用号

    // long (*func)(long *); 函数指针指向的函数
    void *cBegin;       // call begin 在执行系统调用前需要调用的函数
    void *cEnd;         // call end   在执行系统调用后需要调用的函数
};

struct syscall *cbSearch(int id);
int cbInsert(struct syscall *data);

