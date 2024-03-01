#pragma once
#include <stdint.h>
#include <pthread.h>
#include "rbtree.h"
#include "cbargvsdef.h"

/*      Manage Thread Struct    */
typedef struct _InitInfo
{
    pid_t spid;                 // 子进程的pid
    int cfd[2];                 // 用于与‘控制线程’交互的管道
    // 分配给子进程的通讯线程，子进程通过对其进行监控，来达到打断wait进行通讯的目的
    pid_t pid;                  // 线程pid
    pthread_t tid;              // 线程tid
} InitInfo;

typedef enum _ManageType
{
    MT_Start = 10000,
    MT_Init,
    MT_AddPid,               // 新增对进程组的监控
    MT_AddTid,               // 新增对进(线)程的监控
    MT_DelPid,               // 取消对进程组的监控
    MT_DelTid,               // 取消对进(线)程的监控
    MT_CallPass,             // 放过某个调用
    MT_CallDos,              // 拒绝某个调用
    MT_ToExit,               // 退出监控
    MT_End,
} ManageType;
typedef struct _ManageInfo
{
    ManageType type;
    pid_t      pid;
    pid_t      tpid;      // target process
    int        tpfd[2];   // 目标进程的‘控制线程’使用的管道
} ManageInfo;


/*      Mon Proc Struct     */
#define PROC_MAX    16
#define CALL_MAX    512
typedef struct _ControlBaseInfo
{
    pid_t pid;                  // 当前进程的pid
    int tpfd[2];                // 匿名管道，用于向父进程通信
    int bnum;                   // 兄弟进程的数量，一般与cpu核心数量一致
    pid_t bpids[PROC_MAX];      // 兄弟进程的pid，用于忽略监控
    pid_t mainpid;              // 主进程pid
} ControlBaseInfo;
typedef struct _ControlPolicy
{
    ControlBaseInfo binfo;                         // 启动时，由主进程初始化的基本信息
    struct rb_root ftree;                          // 受保护的文件
    struct rb_root dtree;                          // 进程防护树
    int64_t block[CALL_MAX/sizeof(void*)/8];       // 用来判断与bloom对应的系统调用是否需要阻塞
    // 用函数指针的形式以空间换时间，既可以调用，又可以作为布隆过滤器
    // long (*func)(pid_t,long *); 函数指针指向的函数
    long (*cbf[CALL_MAX])(CB_ARGVS_TYPE());        // call begin func     在执行系统调用前需要调用的函数   (4kb)
    long (*cef[CALL_MAX])(CB_ARGVS_TYPE());        // call  end  func     在执行系统调用后需要调用的函数   (4kb)

    // tmp
    int toexit;
} ControlPolicy;
#define SETBLOCK(ControlPolicy,CALLID)              {ControlPolicy->block[CALLID/sizeof(void*)/8] |= 1 << CALLID%(sizeof(void*)*8);}
#define UNBLOCK(ControlPolicy,CALLID)               {ControlPolicy->block[CALLID/sizeof(void*)/8] ^= 1 << CALLID%(sizeof(void*)*8);}
#define ISBLOCK(ControlPolicy,CALLID)               (ControlPolicy->block[CALLID/sizeof(void*)/8] & 1 << CALLID%(sizeof(void*)*8))
#define SETFUNC(ControlPolicy,CALLID,CBF,CEF)       {ControlPolicy->cbf[CALLID] = CBF; ControlPolicy->cef[CALLID] = CEF;}
#define UNSETFUNC(ControlPolicy,CALLID,CBF,CEF)     SETFUNC(ControlPolicy,CALLID,NULL,NULL)
