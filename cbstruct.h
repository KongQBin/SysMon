#pragma once
#include <sys/types.h>
typedef enum _ETYPE
{
    /* 非阻塞
     * 主要用于在某进程使用close关闭某个文件时，将该文件加入非阻塞队列
     * 不关心上层是否返回结果，直接放行
     */
    NBLOCK  = 0,
    /* 阻塞
     * 主要用于在某进程使用execve/at拉起一个新的应用时塞入该队列进行阻塞式扫描
     * 调用回调拿到返回结果，然后选择放行或者拒绝
     */
    BLOCK   = 1,
    /* 特别关注
     * 主要是用于在某个进程使用open/at打开文件时与预先设置的重点文件进行对比
     * 对比成功后使用回调交由上层去处理，根据上层返回的结果选择放行或者拒绝
     */
    HOTFILE = 2,
} ETYPE;

typedef struct _CbMsg
{
    int          ocb;        // 消息源自哪个系统调用
    ETYPE        type;       // 消息模式
    pid_t        otid;       // 消息源自哪个监控线程
    pid_t        gpid;       // 被监控的目标进程组
    pid_t        pid;        // 被监控的目标进程
    // 至于为什么exe与path没有共用一个指针与长度
    // 是因为后期如果有需要可以很轻易的获取到哪个
    // 进程操作了哪个文件，当前为了高效率没有进行实现
    const char   *exe;       // 可执行程序全路径
    int64_t      exelen;     // 可执行程序路径长度
    const char   *opath;     // 源文件全路径
    int64_t      opathlen;   // 源文件路径长度
    const char   *tpath;     // 目标文件全路径
    int64_t      tpathlen;   // 目标文件路径长度
} CbMsg;
typedef int (*MonCb)(CbMsg *info);
