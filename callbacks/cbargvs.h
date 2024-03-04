#pragma once
#include "pidinfo.h"
#include "regsoffset.h"
#include "cbargvsdef.h"
typedef struct _CbArgvs
{
    const PidInfo *info;
    // info中的cinfo，此处主要是为了使用方便
    const ControlPolicy *cinfo;
    // info中的reserve和cctext，此处是为了方便修改
    int *clearContext;
    // 保存在该结构中的变量无需手动释放
    CallContext *cctext;
} CbArgvs;
