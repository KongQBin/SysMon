#include "hotree.h"

struct hotfile* hotSearch(struct rb_root *tree, char *fileid)
{
    if(!tree) return NULL;
    struct hotfile *data;
    rbSearch(tree,searchHotFileCallBack,fileid,data);
    return data;
}

int hotInsert(struct rb_root *tree, struct hotfile *data)
{
    if(!tree || !data) return -1;
    int ret = 0;
    rbInsert(tree,insertHotFileCallBack,data,ret);
    return ret;
}

void hotClear(struct rb_root *tree)
{
    if(!tree) return;
    rbClear(tree,struct hotfile,clearHotFileCallBack);
}

int hotDelete(struct rb_root *tree, char *fileid)
{
    if(!tree) return -1;
    struct hotfile *info = hotSearch(tree,fileid);
    if(!info) return -2;
    rb_erase(&info->node, tree);
    free(info);
    info = NULL;
    return 0;
}

void createFileId(char *fileid, int64_t inode, int64_t devid)
{
//    strcat(fileid,"%llu%llu");
    snprintf(fileid,ID_MAX,"%llu%llu",devid,inode);
    // 倒序一下，将原本的130006789变成987600031
    // 如此在进行字符串对比的时候会节省一大部分时间
    char* start = fileid;
    char* end = &fileid[ID_MAX-1];
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}
