#pragma once
#include "general.h"
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "kmstructs.h"

// 当前线程是‘主进程’与‘监控进程’之间的通讯桥梁
// 主要负责将‘主进程’的消息向‘监控进程‘进行传递
pthread_t createManageThread(InitInfo *info);
int sendManageInfo(ManageInfo *info);
void setTaskOptFunc(void *func);
void onControlThreadMsg(pid_t pid, int status);
