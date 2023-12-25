#pragma once
struct _ThreadData;
struct _Interactive;
typedef struct _CbArgvs
{
    struct pidinfo *info;
    int block;
    struct _ThreadData *td;
    struct _Interactive *task;
} CbArgvs;
/*                  回调函数形参                  */
#define CB_ARGVS_TYPE(av1)            CbArgvs *av1
#define CB_ARGVS                      CB_ARGVS_TYPE(argv)

#include <limits.h>
#include "init.h"
#include "general.h"
//#include "cbdefine.h"   // 各种宏定义，添加系统调用需要添加对应宏
#include "pidtree.h"
#include "managemon.h"
#include "readme.h"     // 首次开发回调函数请先阅读该文档
#include "testfunc.h"
//#include "controlinfo.h"
#include "workthread.h"



// 拒绝(调用)服务
extern long DoS();                         //(内联)获取dos标识
extern long nDoS(long call);               //(内联)临时去除dos标记
extern long cbDoS(CB_ARGVS);  //设置拒绝服务
extern long ceDoS(CB_ARGVS);  //返回拒绝服务

// 获取PID详细信息
extern int getCwd(struct pidinfo *info,char **cwd, size_t *len);                    // cwd  = 当前运行路径                  传入空指针的地址，需要手动去释放
extern int getExe(struct pidinfo *info,char **exe, size_t *len);                    // path = 当前可执行程序路径            传入空指针的地址，需要手动去释放
extern int getFdPath(struct pidinfo *info,long fd, char **path, size_t *len);       // path = 当前被操作的描述符的路径      传入空指针的地址，需要手动去释放
extern int getFdOpenFlag(struct pidinfo *info,long fd, int *flag);                  // flag = 当前被操作描述符的打开权限    传入现有内存/栈区地址

// 获取字符串参数
extern int getStrArg(CbArgvs *argv);
extern int getRegsStrArg(pid_t pid, long arg, char **str, size_t *len);             // arg = 存放参数的寄存器   str = 要获取的参数的指针，传入空指针的地址，需要手动去释放
extern int getRealPath(struct pidinfo *info, char **str, size_t *len);              // str = 传入的是一个在堆区存放 相对路径 字符串的指针的地址，返回存放 绝对路径 的指针

// 提供(监控)服务
extern long cbOpenat(CB_ARGVS);
extern long ceOpenat(CB_ARGVS);

extern long cbWrite(CB_ARGVS);
extern long ceWrite(CB_ARGVS);

extern long cbClose(CB_ARGVS);
extern long ceClose(CB_ARGVS);

extern long cbFork(CB_ARGVS);
extern long ceFork(CB_ARGVS);

extern long cbClone(CB_ARGVS);
extern long ceClone(CB_ARGVS);

extern long cbExecve(CB_ARGVS);
extern long ceExecve(CB_ARGVS);
