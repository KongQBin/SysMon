#pragma once
#include <pthread.h>
#include <dirent.h>
#include "callbacks.h"
#include "general.h"
#include "controlinfo.h"
#include "pidtree.h"
#include "defunc.h"
#define MAX_THREAD_NUM  1

typedef struct _ThreadData
{
    int fd[2];               // 用来向主线程通信的管道
    long *taskNum;          // 全局任务计数器
    void *threadTask[10];   // 当前线程任务池
    // 用于触发主线程条件变量
    pthread_mutex_t *mtx;
    pthread_cond_t *cond;
    // 用于被主线程唤醒的条件变量
    pthread_mutex_t mmtx;
    pthread_cond_t mcond;
    ControlInfo *cInfo;      // 共享数据
} ThreadData;

// 需要帮助子线程做的任务类型
typedef enum _TRACE_TASK_TYPE
{
    TTT_DETACH = 0,     // 取消追踪
    TTT_SETOPTIONS,     // 设置下次关注的事件
    TTT_GETREGS,        // 获取寄存器
    TTT_PEEKDATA,       // 读取寄存器
    TTT_SETREGS,        // 修改寄存器
    TTT_CONT,           // 继续
    TTT_SYSCALL,        // 放行本次调用
} TRACE_TASK_TYPE;
// 获取寄存器
typedef enum _ARGV_TYPE
{
    AT_NUL = 0,
    // int可能用不上，因为数字类型(int short long double等)一般是直接放在寄存器中
    AT_INT,
    // str肯定会用，因为字符串类型放的是字符串所在进程中的地址
    AT_STR,
} ARGV_TYPE;

typedef enum _PROGESS
{
    PG_0 = 0,
    PG_1,
    PG_2,
    PG_3,
} PROGESS;
// 交互数据
// 由子线程发起 到 主线程
// 若是需要返回的任务
// 主线程将初始化好的数据返回 到 子线程
typedef struct _Interactive
{
    pid_t pid;              // 任务pid
    TRACE_TASK_TYPE type;   // 任务类型
    int status;             // 任务状态（用于放行信号）
    PROGESS progess;        // 当前任务被处理的进度
    // 任务是获取寄存器的话才会用以下参数
    int fd[2];              // 主线程向子线程返回消息的管道
    int skip;               // 主线程跳过了这个系统调用
    long *regs;             // 寄存器地址,size应该是struct user中regs的大小
    ARGV_TYPE argvType[6];  // 参数1-6的类型
    long argv[6];           // 参数1-6的值，对于指针类要强转
    long argvLen[6];        // 如果是字符串参数的话，这里是对应的长度
} Interactive;

typedef struct _TaskOrigin
{
    pid_t *pids;            // 指向的内存应该与线程数量一致
    unsigned long len;      // 当前可用索引位置
    unsigned long maxLen;   // 最大长度
} TaskOrigin;

void* workThread(void* pinfo);
