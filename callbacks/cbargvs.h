#pragma once
#include "pidinfo.h"
#include "regsoffset.h"
#include "cbargvsdef.h"
typedef enum _CArgvsType
{
    CAT_LONG,        // int uint long ulong short float double
    CAT_STRING,      // char *str
    CAT_BYTEARRAY,   // char *byte
} CArgvsType;
typedef struct _CallContext
{
    long *regs;
    CArgvsType types[REGS_NUMBER];
    // 如果对应的参数是[非数字]类型，那么将其赋值为一个char*指针
    long argvs[REGS_NUMBER];
    // 如果对应的参数是[非数字]类型，则用来存放长度
    long argvsLen[REGS_NUMBER];

} CallContext;
typedef struct _CbArgvs
{
    // 用来获取进程关系(不需要在cb/eFunc中去释放或释放其内部成员)
    PidInfo *info;
    // 当前系统调用的上下文
    CallContext cctext;
} CbArgvs;
