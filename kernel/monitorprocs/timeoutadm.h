#pragma once
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include "kmstructs.h"
#include "general.h"

// wfd：用来通知解除挂起状态的管道写端描述符
int startTimeoutAdmThread(const int *wfd);
int preStopTimeoutAdmThread();
int stopTimeoutAdmThread();
// 用来向添加新的挂起信息
int addPinfo(const pid_t pid);
// 用来删除挂起信息（在未超时的强况下手动删除）
int delPinfo(const pid_t pid, int index, int locked);
