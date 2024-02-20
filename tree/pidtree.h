#pragma once

/* 在Linux中线程其实是通过轻量级进程实现的，也就是LWP(light weight process)
 * 因此在Linux中每个线程都是一个进程，都拥有一个PID
 * 换句话说，操作系统原理中的线程，对应的其实是Linux中的进程(即LWP)
 * 因此Linux内核中的PID对应的其实是原理中的TID。
 *
 * 操作系统原理中的进程对应的其实是Linux中的线程组，准确的叫法应该是轻量级进程组，
 * 但是由于Linux中轻量级进程就是线程，所以一般叫做Linux线程组
 * 同一Linux线程组的线程(LWP)，是共享内存地址空间的，即共享页表。
 *
 * 而Linux线程组中的主线程的PID，在数值上等于该线程组的TGID，这个TGID对应了原理中的PID。
 *
 * 总的来说就是内核中不存在线程，应用层线程就是内核层进程，应用层进程就是内核层进程组
 */
#include "general.h"
#include "rbtree.h"
#include "rbdef.h"
#include <stdlib.h>
#include <stdio.h>

// 被监控的进程信息
struct _ControlInfo;
typedef struct _PidInfo
{
    struct rb_node node;
    pid_t pid;                  //进程ID
    pid_t gpid;                 //进程属于哪个进程组
    pid_t ppid;                 //进程的父进程组（目前只监控追踪之后确认的关系，之前的默认为0）
    char *exe;                  //进程的可执行程序路径
    size_t exelen;

    int64_t flags;                      //setopt、stop状态等
    struct _ControlInfo *cinfo;         //当要对这个进程进行独立管理时，该指针将指向新的内存，否则指向gDefaultControlInfo

    int status;         // 当前进程返回的状态
} PidInfo;
#define PID_STOP        0
#define PID_SETOPT      1
#define PID_CHKWHITE    2   // 已过白

#define SET_STOP(flags)         (flags |= 1<<PID_STOP)
#define SET_SETOPT(flags)       (flags |= 1<<PID_SETOPT)
#define SET_CHKWHITE(flags)     (flags |= 1<<PID_CHKWHITE)
#define UNSET_STOP(flags)       (flags &= ~(1<<PID_STOP))
#define UNSET_SETOPT(flags)     (flags &= ~(1<<PID_SETOPT))
#define IS_STOP(flags)          (flags & (1<<PID_STOP))
#define IS_SETOPT(flags)        (flags & (1<<PID_SETOPT))
#define CHKWHITED(flags)        (flags & (1<<PID_CHKWHITE))



static inline int searchPidInfoCallBack(PidInfo *info,int pid,int opt)
{
//    printf("info = %x\n",info);
//    printf("info.pid = %d\n",info->pid);
    return opt ? info->pid < pid : info->pid > pid;
}
static inline int insertPidInfoCallBack(PidInfo *info1,PidInfo *info2,int opt)
{
//    printf("info1 = %x\tinfo1.pid = %d\n",info1,info1->pid);
//    printf("info2 = %x\tinfo2.pid = %d\n",info2,info2->pid);
    return opt ? info1->pid > info2->pid : info1->pid < info2->pid;
}
static inline void clearPidInfoCallBack(PidInfo *info){free(info); info = NULL;}

PidInfo* pidSearch(struct rb_root *tree,pid_t pid);      // 查询
int pidInsert(struct rb_root *tree,PidInfo *data);       // 插入
int pidDelete(struct rb_root *tree,pid_t pid);                  // 删除
void pidClear(struct rb_root *tree);                            // 清空
int64_t pidTreeSize(struct rb_root *tree);
PidInfo* createPidInfo(pid_t pid,pid_t gpid,pid_t ppid); // 创建新的pidinfo

void resetItNode();
struct rb_node* iterateNode(struct rb_root *tree);
PidInfo* getStruct(struct rb_node* node);
