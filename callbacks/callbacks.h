#pragma once
#include "kmstructs.h"
#include "regsoffset.h"
#include "cbargvs.h"
#include <limits.h>
#include "init.h"
#include "general.h"
#include "pidtree.h"
#include "managemon.h"
#include "readme.h"     // 首次开发回调函数请先阅读该文档
#include "testfunc.h"


// 获取PID详细信息
extern int getCwd(const PidInfo *info,char **cwd, size_t *len);                    // cwd   = 当前运行路径                  传入空指针的地址，需要手动去释放
extern int getExe(const PidInfo *info,char **exe, size_t *len);                    // path  = 当前可执行程序路径            传入空指针的地址，需要手动去释放
extern int getFdPath(const PidInfo *info,long fd, char **path, size_t *len);       // path  = 当前被操作的描述符的路径      传入空指针的地址，需要手动去释放
extern int getFdOpenFlag(const PidInfo *info,long fd, int *flags);                 // flags = 当前被操作描述符的打开权限    传入现有内存/栈区地址
/*
 * 获取内存中的参数
 * @pid： 要对哪个进程进行拷贝
 * @originaddr： 位于目标进程的起始地址
 * @targetaddr： 要拷贝到的目标地址[需手动释放]
 * @len： 要拷贝的长度
 * @ret： 成功返回0
 * PS： 如果参数4=NULL,则表示要拷贝的是字符串，
 * 此时参数3将被自动开辟,否则需要手动开辟并传入
*/
extern int getArg(const pid_t *pid, const long *originaddr, void **targetaddr, size_t *len);
/*
 * 获取绝对路径
 * 将相对路径与进程工作路径进行拼接，
 * 并翻译多余的{/../、/./、//}等符号
 * @info：相对路径来自哪个进程
 * @str：返回路径
 * @len：返回路径长度
 * @ret：成功返回0
*/
extern int getRealPath(const PidInfo *info, char **str, size_t *len);

#define EXTERN_FUNC(name,argvs) \
extern long cb##name(argvs); \
extern long ce##name(argvs);

// 拒绝(调用)服务
extern long DoS();                         // (内联)获取dos标识
extern long nDoS(long call);               // (内联)临时去除dos标记
EXTERN_FUNC(DoS,CB_ARGVS)                  // 设置与返回拒绝服务
// 提供(监控系统调用)服务
EXTERN_FUNC(Openat,CB_ARGVS)
EXTERN_FUNC(Close,CB_ARGVS)
EXTERN_FUNC(Execve,CB_ARGVS)
EXTERN_FUNC(Rename,CB_ARGVS)
EXTERN_FUNC(Renameat,CB_ARGVS)
EXTERN_FUNC(Renameat2,CB_ARGVS)

EXTERN_FUNC(Fork,CB_ARGVS)
EXTERN_FUNC(Clone,CB_ARGVS)
EXTERN_FUNC(Write,CB_ARGVS)
