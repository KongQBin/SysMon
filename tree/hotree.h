#pragma once
#include "rbtree.h"
#include "rbdef.h"
#include <stdlib.h>
#include <stdio.h>

//extern struct rb_root g_cbTree;
struct hotfile
{
    struct rb_node node;
    int64_t   inode;           // 文件inode
    int64_t   devid;           // 文件所属devid
};

static inline int searchCallBack(struct hotfile *file,int inode,int opt)
{return opt ? file->inode < inode : file->inode > inode;}
static inline int insertCallBack(struct hotfile *file1,struct hotfile *file2,int opt)
{return opt ? file1->inode > file2->inode : file1->inode < file2->inode;}
static inline void clearCallBack(struct hotfile *file){free(file); file = NULL;}

struct hotfile* hotSearch(struct rb_root *tree,int inode);   //查询
int hotInsert(struct rb_root *tree,struct hotfile *data); //插入
void hotClear(struct rb_root *tree);                     //清空

