#pragma once
#include <sys/types.h>
#include <sys/wait.h>
#include "kmstructs.h"
#include "ptraceopt.h"
#include "pglobal.h"
#include "timeoutadm.h"
// 用来处理来自‘控制线程’的任务
void taskOpt(ManageInfo *minfo,ControlBaseInfo *cbinfo);
