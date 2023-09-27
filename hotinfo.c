#include "hotinfo.h"
struct rb_root *hotTree = NULL;
struct rb_root *createHotTree()
{
    hotTree = calloc(1,sizeof(struct rb_root));
    return hotTree;
}

void deleteInitHotTree()
{
    if(hotTree) hotClear(hotTree);
}

int insertHotTree(struct hotfile *file)
{
    return hotInsert(hotTree,file);
}

struct hotfile *searchHotTree(int64_t inode)
{
    return hotSearch(hotTree,inode);
}
