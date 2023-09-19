#pragma once
#include "rbtree.h"
#include "rbdef.h"
#include <stdlib.h>
#include <stdio.h>

//extern struct rb_root g_cbTree;
struct syscall
{
    struct rb_node node;
    int   id;           // 系统调用号
    // long (*func)(pid_t,long *); 函数指针指向的函数
    void *cbf;          // call begin func     在执行系统调用前需要调用的函数
    void *cef;          // call end   func     在执行系统调用后需要调用的函数
};

static inline int searchCallBack(struct syscall *call,int id,int opt)
{return opt ? call->id < id : call->id > id;}
static inline int insertCallBack(struct syscall *call1,struct syscall *call2,int opt)
{return opt ? call1->id < call2->id : call1->id > call2->id;}
static inline void clearCallBack(struct syscall *call){free(call); call = NULL;}

struct syscall* cbSearch(struct rb_root *tree,int id);   //查询
int cbInsert(struct rb_root *tree,struct syscall *data); //插入
void cbClear(struct rb_root *tree);                     //清空

