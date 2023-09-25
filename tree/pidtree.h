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
 */
#include "rbtree.h"
#include "rbdef.h"
#include <stdlib.h>
#include <stdio.h>

struct pidinfo
{
    struct rb_node node;
    pid_t id;           //当前被监控的线程ID（也可能是进程）
    pid_t origin_id;    //该线程创建自哪个进程
    pid_t trace_id;     //追踪者，也就是我们的线程ID
};

static inline int searchCallBack(struct pidinfo *info,int id,int opt)
{return opt ? info->id < id : info->id > id;}
static inline int insertCallBack(struct pidinfo *info1,struct pidinfo *info2,int opt)
{return opt ? info1->id > info2->id : info1->id < info2->id;}
static inline void clearCallBack(struct pidinfo *info){free(info); info = NULL;}

struct pidinfo* hotSearch(struct rb_root *tree,int id);      //查询
int hotInsert(struct rb_root *tree,struct pidinfo *data);    //插入
void hotClear(struct rb_root *tree);                         //清空
