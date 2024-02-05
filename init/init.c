#include "init.h"
RegsOffet g_regsOffset;
extern long DoS();
extern int getRegsOffset();       // 初始化寄存器偏移
int initRegsOffset()
{
    int ret = getRegsOffset();
    DMSG(ML_INFO,"current process id = %d\n",getpid());
    DMSG(ML_INFO,"initRegsOffset ret = %d\n",ret);
    DMSG(ML_INFO,"offset.call = %d\n",g_regsOffset.call);
    DMSG(ML_INFO,"offset.ret = %d\n",g_regsOffset.ret);
    DMSG(ML_INFO,"offset.argv1 = %d\n",g_regsOffset.argv1);
    DMSG(ML_INFO,"offset.argv2 = %d\n",g_regsOffset.argv2);
    DMSG(ML_INFO,"offset.argv3 = %d\n",g_regsOffset.argv3);
    return ret;
}

int getRegsOffset()
{
    int ret = -1;
    const char *msg = "test write";
    const char *msg2 = "abcdefghijklmn";
    int msgLen = strlen(msg);
    int msgLen2 = strlen(msg2);

    int fd[2] = {0};
    if(pipe(fd) < 0)
    {
        DMSG(ML_ERR,"pipe err : %s\n",strerror(errno));
        return -2;
    }

    fcntl(fd[0],F_SETFL,fcntl(fd[0],F_GETFL)|O_NONBLOCK);//设置fd为阻塞模式
//    fcntl(fd[1],F_SETFL,fcntl(fd[0],F_GETFL)|O_NONBLOCK);

    int pr = fd[0], pw = fd[1];
    pid_t pid = fork();
    if(pid == 0)
    {
        DMSG(ML_INFO,"son pid is %d\n",getpid());
        int fd = open("/dev/null",O_WRONLY);
        if(fd < 0) DMSG(ML_ERR,"open : %s\n",strerror(errno));
        if(fd >= 0 && write(pw,&fd,sizeof(fd)) == sizeof(fd))
        {
            int whileNum = 10;
            while(--whileNum)
            {
                usleep(100000);
                write(fd,msg,msgLen);
                write(fd,msg2,msgLen2);
            }
        }
        if(fd >= 0) close(fd);
        if(pr >= 0) close(pr);
        if(pw >= 0) close(pw);
        exit(0);
    }
    else if(pid > 0)
    {
        usleep(300000);
        if (ptrace(PTRACE_ATTACH, pid, 0, 0) == -1) {
            DMSG(ML_ERR,"ptrace attach : %s\n",strerror(errno));
            return -3;
        }
//        if(ptrace(PTRACE_SYSEMU, pid, 0, 0) == -1) {
////            DMSG(ML_ERR,"ptrace sysemu : %s\n",strerror(errno));
////            return -4;
//        }

//        struct user_regs_struct regs;
        struct user user;
        int regsNum = sizeof(user.regs)/sizeof(long);

        int sonFd = -1;
        if(read(pr,&sonFd,sizeof(sonFd)) != sizeof(sonFd))
            return -5;

//        DMSG(ML_INFO,"sonFd is %d\n",sonFd);
        int whileNum = 20;
        memset(&g_regsOffset,-1,sizeof(g_regsOffset));
        while(--whileNum)
        {
            wait(0);

            if(!ret)
            {
                if(ptrace(PTRACE_DETACH, pid, 0, 0) == -1 ) {
                    DMSG(ML_ERR,"ptrace detach : %s\n",strerror(errno));
                }
                break;
            }

            memset(&user.regs,0,sizeof(user.regs));
            ptrace(PT_GETREGS, pid, 0, &user.regs);
            printUserRegsStruct2(&user);

            long *pRegs = (long*)&user.regs;
            for(int i=0;i<regsNum;++i)
            {
                // 命中返回值
                if(g_regsOffset.ret == -1
                    && pRegs[i] == -38) g_regsOffset.ret = i;
                else
                {
                    // 命中第一个参数
                    if(g_regsOffset.argv1 == -1
                        && pRegs[i] == sonFd) g_regsOffset.argv1 = i;
                    // 命中第二个参数
                    if(g_regsOffset.argv2 == -1)
                    {
                        long tmp = ptrace(PTRACE_PEEKDATA, pid, pRegs[i], NULL);
                        if(!memcmp(&tmp,msg,sizeof(long))
                            || !memcmp(&tmp,msg2,sizeof(long)))
                            g_regsOffset.argv2 = i;
                    }
                    // 命中第三个参数
                    if(g_regsOffset.argv3 == -1
                        && (pRegs[i] == msgLen
                            || pRegs[i] == msgLen2))
                    {
                        g_regsOffset.argv3 = i;
                    }
                }
            }

            if(g_regsOffset.ret != -1 && g_regsOffset.argv1 != -1
                && g_regsOffset.argv2 != -1 && g_regsOffset.argv3 != -1)
            {
                for(int j=0;j<regsNum;++j)
                {
                    // 命中系统调用号
                    if(j != g_regsOffset.ret && j != g_regsOffset.argv1
                        && j != g_regsOffset.argv2 && j != g_regsOffset.argv3)
                    {
                        // write调用号 32位系统等于4 64位系统等于1
                        if(pRegs[j]%1000 == ID_WRITE)
                        {
                            g_regsOffset.call = j;
                            DMSG(ML_INFO,"call id is %d\n",pRegs[j]);
                            break;
                        }
                    }
                }
            }

            if(g_regsOffset.call != -1) ret = 0;
            if(ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) {
                DMSG(ML_ERR,"ptrace syscall : %s\n",strerror(errno));
                ret = -6;
            }
        }
        int status = 0;
        waitpid(pid,&status,0);
    }
    else
    {
        DMSG(ML_ERR,"fork : %s\n",strerror(errno));
    }
    if(pr >= 0) close(pr);
    if(pw >= 0) close(pw);
    return ret;
}

