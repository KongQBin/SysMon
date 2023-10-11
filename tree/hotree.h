#pragma once
#include "rbtree.h"
#include "rbdef.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//extern struct rb_root g_cbTree;
#define ID_MAX 40
struct hotfile
{
    struct rb_node node;
    char   fileid[ID_MAX];         // 文件id，由inode和devid拼接
//    int64_t   inode;           // 文件inode
//    int64_t   devid;           // 文件所属devid
    int       oflag;           // 要关注的打开权限，使用方式与open函数的第二个参数一致
};


static inline int searchHotFileCallBack(struct hotfile *file, char *fileid,int opt)
//{return opt ? file->inode < inode : file->inode > inode;}
{return opt ? strncmp(file->fileid,fileid,ID_MAX) < 0 : strncmp(file->fileid,fileid,ID_MAX) > 0;}
static inline int insertHotFileCallBack(struct hotfile *file1,struct hotfile *file2,int opt)
//{return opt ? file1->inode > file2->inode : file1->inode < file2->inode;}
{return opt ? strncmp(file1->fileid,file2->fileid,ID_MAX) > 0 : strncmp(file1->fileid,file2->fileid,ID_MAX) < 0;}
static inline void clearHotFileCallBack(struct hotfile *file){free(file); file = NULL;}

void createFileId(char *fileid, int64_t inode, int64_t devid);
struct hotfile* hotSearch(struct rb_root *tree,char *fileid);   // 查询
int hotInsert(struct rb_root *tree,struct hotfile *data);        // 插入
int hotDelete(struct rb_root *tree, char *fileid);               // 删除
void hotClear(struct rb_root *tree);                             // 清空

