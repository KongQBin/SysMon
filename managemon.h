#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hotinfo.h"
enum ORIGIN
{
    /* 特别关注
     * 主要是用于在某个进程使用open/at打开文件时与预先设置的重点文件进行对比
     * 对比成功后使用回调交由上层去处理，根据上层返回的结果选择放行或者拒绝
     */
    HOTFILE = 0,
    /* 非阻塞
     * 主要用于在某进程使用close关闭某个文件时，将该文件加入非阻塞队列
     * 不关心上层是否返回结果，直接放行
     */
    NBLOCK  = 1,
    /* 阻塞
     * 主要用于在某进程使用execve/at拉起一个新的应用时塞入该队列进行阻塞式扫描
     * 调用回调拿到返回结果，然后选择放行或者拒绝
     */
    BLOCK   = 2
};
struct BaseInfo
{
    pthread_t   otid;       // origin tid（消息源自哪个监控线程）
    pid_t       tpid;       // target pid（被监控的目标进程）
    pthread_t   ttid;       // target tid（被监控的目标进程中的哪个线程）
    char        *path;      // 文件全路径
    int64_t     pathlen;    // 路径长度
    enum ORIGIN origin;     // 消息来源
};

int (*CbInfoPut)(BaseInfo *info) = NULL;
