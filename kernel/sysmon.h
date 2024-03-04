#pragma once
#include <dirent.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/sysctl.h>
#include "defunc.h"
#include "testfunc.h"
#include "procmain.h"
#include "managethread.h"
#include "iteratesys.h"
//#include "pidtree.h"
//#include "general.h"
//#include "callbacks.h"
//#include "controlinfo.h"


typedef enum _MonProcType
{
    MPT_Exit,
} MonProcType;

typedef struct _MonProc
{
    MonProcType type;
} MonProc;

typedef enum _OutsideType
{
    OT_AdmMain, // 主进程管理消息
    OT_AdmMon,  // ‘监控进程’管理消息
} OutsideType;
typedef struct _Outside
{
    OutsideType type;
    // AdmMon
    ManageInfo info;
    // AdmMain
    int toExit;
} Outside;
typedef enum _MainDataOrigin
{
    MDO_MonProc,        // 消息来自‘监控(子)进程’
    MDO_Outside,        // 消息来自模块外部
} MDOrigin;
typedef struct _MainData
{
    MDOrigin origin;
    MonProc monproc;
    Outside outside;
} MData;
int StartSystemMonitor(MonCb callback);
