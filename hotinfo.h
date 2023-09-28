#pragma once
#include <unistd.h>
#include <stdlib.h>
#include "hotree.h"

extern struct rb_root *hotTree;
// 传空新建并返回，传非空赋值并返回
struct rb_root* createHotTree();
int insertHotTree(struct hotfile *file);
struct hotfile *searchHotTree(int64_t inode,int64_t devid);
void deleteInitHotTree();
