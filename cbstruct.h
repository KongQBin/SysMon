#pragma once
#include <sys/types.h>
typedef enum _ETYPE
{

    // 非阻塞：主要用于文件监控
    NBLOCK  = 0,
    // 阻塞：主要用于进程监控
    BLOCK   = 1,
    /* 特别关注
     * 主要是用于在某个进程使用open/at打开文件时与预先设置的重点文件进行对比
     * 对比成功后使用回调交由上层去处理，根据上层返回的结果选择放行或者拒绝
     */
    HOTFILE = 2,
} ETYPE;

typedef struct _CbMsg
{
    pid_t        otid;       // 消息源自哪个监控进程
    int          wfd;        // 匿名管道写端，用于将控制消息传递给监控进程

    int          ocb;        // 消息源自哪个系统调用
    ETYPE        type;       // 消息类型
    pid_t        gpid;       // 被监控的目标进程组
    pid_t        pid;        // 被监控的目标进程
    const char   *exe;       // 可执行程序全路径
    int64_t      exelen;     // 可执行程序路径长度
    const char   *opath;     // 源文件全路径
    int64_t      opathlen;   // 源文件路径长度
    const char   *tpath;     // 目标文件全路径
    int64_t      tpathlen;   // 目标文件路径长度
} CbMsg;
typedef int (*MonCb)(CbMsg *info);
