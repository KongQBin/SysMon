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
    int *reserveContext;
    CallContext *cctext;
} CbArgvs;
