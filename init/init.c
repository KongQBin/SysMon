#include "init.h"
RegsOffet gRegsOffset;
extern int gSeize;
extern long DoS();
extern int getRegsOffset();       // 初始化寄存器偏移
int initRegsOffset()
{
    int ret = getRegsOffset();
    DMSG(ML_INFO,"Init regs offset %s.\n",ret ? "Fail" : "Success");
    DMSG(ML_INFO,"Current tracking mode %s.\n",gSeize ? "SEIZE" : "ATTACH");
    DMSG(ML_INFO,"Current process id = %d.\n",getpid());
    DMSG(ML_INFO,"Offset:\n");
    DMSG(ML_INFO,"> ret func(argv1,argv2,argv3,argv4,argv5).\n");
    DMSG(ML_INFO,">   ^    ^(    ^,    ^,    ^,    ^,    ^).\n");
    DMSG(ML_INFO,"> %3d %4d(%5d,%5d,%5d,%5d,%5d).\n",
         gRegsOffset.ret,gRegsOffset.call,gRegsOffset.argv1,gRegsOffset.argv2,
         gRegsOffset.argv3,gRegsOffset.argv4,gRegsOffset.argv5);
    return ret;
}

typedef struct _UseCaseData
{
    long oldfd,newfd,flags;
    char oldname[8], newname[8];
} UseCaseData;

// 经尝试，通过对子线程的追踪初始化寄存器偏移，不可行
// 进行附加时会抛出错误：Operation not permitted
// 故完善并使用下方getRegsOffset对子进程进程追踪
int getRegsOffset()
{
    int ret = -1;
    int fd[2] = {0};
    if(pipe(fd) < 0)
    {
        DMSG(ML_ERR,"pipe err : %s\n",strerror(errno));
        return -2;
    }

    int pr = fd[0], pw = fd[1];
    UseCaseData *data = NULL;
    pid_t pid = fork();
    if(pid == 0)
    {
        do{
            data = calloc(1,sizeof(UseCaseData));
            if(!data) break;
            data->oldfd = 0xDEADBEEF;
            data->newfd = 0xCAFEBABE;
            data->flags = 0x0BADF00D;
            strcpy(data->oldname,"nixnehc");
            strcpy(data->newname,"nibgnok");

            // 通知主进程我要使用的参数，随后进行调用
            if(write(pw,data,sizeof(UseCaseData)) == sizeof(UseCaseData))
            {
                int whileNum = 20;
                while(--whileNum)
                {
                    usleep(100000);
                    renameat2(data->oldfd,data->oldname,
                              data->newfd,data->newname,data->flags);
                }
            }
        }while(0);
        if(data) free(data);
        if(pr >= 0) close(pr);
        if(pw >= 0) close(pw);
        exit(0);
    }
    else if(pid > 0)
    {
        usleep(300000);
        do{
            data = calloc(1,sizeof(UseCaseData));
            if(!data)
            {ret = -3; break;}
            if(read(pr,data,sizeof(UseCaseData)) != sizeof(UseCaseData))
            {ret = -4; break;}

            if(ptrace(PTRACE_SEIZE,pid,0,PTRACE_O_TRACESYSGOOD))
            {
                if(ptrace(PTRACE_ATTACH, pid, 0, 0))
                {
                    DMSG(ML_ERR,"ptrace attach : %s\n",strerror(errno));
                    ret = -5;
                    break;
                }
            }
            else
            {
                if(ptrace(PTRACE_INTERRUPT, pid, 0L, 0L))
                    DMSG(ML_ERR,"ptrace interrupt : %s\n",strerror(errno));
                // 切换为SEIZE模式
                gSeize = 1;
            }

            struct user user;
            int regsNum = sizeof(user.regs)/sizeof(long);
            int whileNum = 20,toContinue = 0;
            memset(&gRegsOffset,-1,sizeof(gRegsOffset));
            while(--whileNum >= 0)
            {
                waitpid(pid,NULL,WUNTRACED);
                memset(&user.regs,0,sizeof(user.regs));
                ptrace(PT_GETREGS, pid, 0, &user.regs);
                printUserRegsStruct2(&user);

                long *pRegs = (long*)&user.regs;
                for(int i=0;i<regsNum;++i)
                {
                    // 排除已经确认的寄存器
                    toContinue = 0;
                    for(int j=0; j<sizeof(gRegsOffset)/sizeof(int) -1/*减去参数6*/; ++j)
                    {
                        int *pOffset = (int*)&gRegsOffset;
                        if(pOffset[j] == i)
                        {
                            toContinue = 1;
                            break;
                        }
                    }
                    if(toContinue) continue;
                    // 开始判断并初始化各个寄存器的偏移
                    do{

                        long tmp = ptrace(PTRACE_PEEKDATA, pid, pRegs[i], NULL);
                        // 命中返回值
                        if(pRegs[i] == -38)
                        {
                            gRegsOffset.ret = i;
                            break;
                        }

                        // 命中系统调用
                        if(pRegs[i] == __NR_renameat2)
                        {
                            gRegsOffset.call = i;
                            break;
                        }

                        // 命中第一个参数
                        if(pRegs[i] == data->oldfd)
                        {
                            gRegsOffset.argv1 = i;
                            break;
                        }

                        // 尝试判断第二个参数
                        if(!strncmp((char*)&tmp,data->oldname,sizeof(long)))
                        {
                            gRegsOffset.argv2 = i;
                            break;
                        }

                        // 命中第三个参数
                        if(pRegs[i] == data->newfd)
                        {
                            gRegsOffset.argv3 = i;
                            break;
                        }

                        // 尝试判断第四个参数
                        if(!strncmp((char*)&tmp,data->newname,sizeof(long)))
                        {
                            gRegsOffset.argv4 = i;
                            break;
                        }

                        // 命中第五个参数
                        if(pRegs[i] == data->flags)
                        {
                            gRegsOffset.argv5 = i;
                            break;
                        }
                    }while(0);
                }

                // 检查初始化进度
                toContinue = 0;
                for(int j=0; j<sizeof(gRegsOffset)/sizeof(int) -1/*减去参数6*/; ++j)
                {
                    int *pOffset = (int*)&gRegsOffset;
                    if(pOffset[j] == -1)
                        toContinue = 1;
                }

                if(!toContinue)
                {
                    // 全部初始化完成，结束循环
                    if(ptrace(PTRACE_DETACH, pid, 0, 0))
                        DMSG(ML_ERR,"ptrace detach : %s\n",strerror(errno));
                    ret = 0;
                    break;
                }
                else
                    // 未初始化完成，继续下一次循环
                    if(ptrace(PTRACE_SYSCALL, pid, 0, 0))
                        DERR(PTRACE_SYSCALL);
            }
        }while(0);
    }
    else
    {
        DMSG(ML_ERR,"fork : %s\n",strerror(errno));
    }

    if(data) free(data);
    if(pr >= 0) close(pr);
    if(pw >= 0) close(pw);
    return ret;
}

