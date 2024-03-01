#pragma once
typedef struct _RegsOffet
{
    int ret;        //返回值
    int call;       //系统调用号
    int argv1;      //参数一
    int argv2;      //参数二
    int argv3;      //参数三
    int argv4;      //参数四
    int argv5;      //参数五
    int argv6;      //参数六
} RegsOffet;
extern RegsOffet gRegsOffset;
#define REGS_NUMBER sizeof(RegsOffet)/sizeof(int)
//#define RET(regs)    regs[gRegsOffset.ret]
//#define CALL(regs)   regs[gRegsOffset.call]
//#define ARGV_1(regs) regs[gRegsOffset.argv1]
//#define ARGV_2(regs) regs[gRegsOffset.argv2]
//#define ARGV_3(regs) regs[gRegsOffset.argv3]
//#define ARGV_4(regs) regs[gRegsOffset.argv4]
//#define ARGV_5(regs) regs[gRegsOffset.argv5]
//#define ARGV_6(regs) regs[gRegsOffset.argv6]
#define AO_RET    0
#define AO_CALL   1
#define AO_ARGV1  2
#define AO_ARGV2  3
#define AO_ARGV3  4
#define AO_ARGV4  5
#define AO_ARGV5  6
#define AO_ARGV6  7
#define RET(regs)    regs[((int*)&gRegsOffset)[AO_RET]]
#define CALL(regs)   regs[((int*)&gRegsOffset)[AO_CALL]]
#define ARGV_1(regs) regs[((int*)&gRegsOffset)[AO_ARGV1]]
#define ARGV_2(regs) regs[((int*)&gRegsOffset)[AO_ARGV2]]
#define ARGV_3(regs) regs[((int*)&gRegsOffset)[AO_ARGV3]]
#define ARGV_4(regs) regs[((int*)&gRegsOffset)[AO_ARGV4]]
#define ARGV_5(regs) regs[((int*)&gRegsOffset)[AO_ARGV5]]
#define ARGV_6(regs) regs[((int*)&gRegsOffset)[AO_ARGV6]]
