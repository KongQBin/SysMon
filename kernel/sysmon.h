#pragma once
#include <sys/epoll.h>
//#include <pthread.h>
//#include <dirent.h>
//#include "callbacks.h"
//#include "general.h"
#include "defunc.h"
#include "testfunc.h"
#include "procinfo.h"
#include "workthread.h"
//#include "controlinfo.h"
//#include "pidtree.h"



// ptrace使用了PTRACE_O_TRACEFORK就自动附加了子进程了，无法拉起子线程再进行附加了，没办法并发
/* 解决思路一：while(wait)仅负责遍历进程及其子进程事件*/
/*
 *<-----------------------------------------------------------------------------------------核心处理逻辑（阻塞模式）----------------------------------------------------------------------------------------->
 *                                                                               SysMon                                                                                     外部其它进程/线程
 *
 *     主线程                                                                  某个子线程
 *     while(1)                         while(1)                                                                        signal(sigusr1)                                      收到SysMon的通知
 *     {                                {                                                                               {                                                           :
 *             read(控制信息)                       if(hasFinish)                                                           if(working)                                             V
 *                   :                                    :                                                                            :                                   读取共享内存中的信息
 *                   V                                    V                                                                            V                                            :
 *               得到线程ID              遍历共享内存，放行/拒绝已处理结束的进程                                                  hasFinish = 1                                     V
 *                   :                                    :                                                                            :                              扫描目标共享内存中所提供的文件路径
 *                   V                                    V                                                                            V                                            :
 *       pthread_kill(线程ID,sigusr1)              atomic working = 0                                                                return                                         V
 *     }                                                  :                                                                  else                                  根据扫描结果通知SysMon的主线程允许或拒绝
 *                                                        V                                                                            :
 *                                                     wait(-1)                                                                        V
 *                                                        :                                                                   根据共享内存中的信息
 *                                                        V                                                                            :
 *                                                 atomic working = 1                                                                  V
 *                                                        :                                                                放行/拒绝目标进程的系统调用
 *                                                        V                                                                            :
 *                                                    分析事件--------------------->需阻塞(exec...)                                    V
 *                                                        :                                :                                    标记已处理该信息
 *                                                        V                                :                            }
 *                                                无需阻塞(close...)                       :
 *                                                        :                                :
 *                                                        V                                V
 *                                              文件信息加入非阻塞队列    进程信息加入共享内存(并通知外部进程或线程)
 *                                                        :                                :
 *                                                        V                                :
 *                                                       放行                              :
 *                                                        :                                :
 *                                                        V                                :
 *                                            continue(去等待其它进程事件) <---------------'
 *                                        }
 * pthread_kill信号必须发往子线程，让子线程的信号处理函数去处理，因为ptrace不支持多线程调试
 * 使用信号的核心作用主要就是通知子线程有新的任务，同时在子线程wait阻塞时，它能够打断wait
 */

void* startMon(void* ppid);
void* newStartMon(void* pinfo);
