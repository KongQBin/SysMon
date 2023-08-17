#include "init.h"
int init()
{
    int ret = initRegsOffset();
    printf("current process id = %d\ninitRegsOffset ret = %d\n",getpid(),ret);
    printf("offset.call = %d\n",g_regsOffset.call);
    printf("offset.ret = %d\n",g_regsOffset.ret);
    printf("offset.argv1 = %d\n",g_regsOffset.argv1);
    printf("offset.argv2 = %d\n",g_regsOffset.argv2);
    printf("offset.argv3 = %d\n",g_regsOffset.argv3);
    return ret;
}

int initRegsOffset()
{
    int ret = 0;
    const char *msg = "test write";
    const char *msg2 = "abcdefghijklmn";
    int msgLen = strlen(msg);
    int msgLen2 = strlen(msg2);


    int fd[2] = {0};
    if(pipe(fd) < 0)
    {
        printf("pipe err : %s\n",strerror(errno));
        return -1;
    }

    fcntl(fd[0],F_SETFL,fcntl(fd[0],F_GETFL)|O_NONBLOCK);//设置fd为阻塞模式
    fcntl(fd[1],F_SETFL,fcntl(fd[0],F_GETFL)|O_NONBLOCK);

    int pr = fd[0], pw = fd[1];
    pid_t pid = fork();
    if(pid == 0)
    {
//        printf("son pid is %d\n",getpid());
        int fd = open("/dev/null",O_WRONLY);
        if(fd < 0) perror("open");
        if(fd >= 0 && write(pw,&fd,sizeof(fd)) == sizeof(fd))
        {
            int whileNum = 10;
            while(--whileNum)
            {
//                printf("while = %d\n",whileNum);
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
//        printf("son pid is %d\n",pid);
        if (ptrace(PTRACE_ATTACH, pid, 0, 0) == -1) {
            perror("ptrace attach");
            return -2;
        }
//        if(ptrace(PTRACE_SYSEMU, pid, 0, 0) == -1) {
////            perror("ptrace sysemu");
////            return -3;
//        }

        struct user_regs_struct regs;
        int regsNum = sizeof(regs)/sizeof(long);

        int sonFd = -1;
        if(read(pr,&sonFd,sizeof(sonFd)) != sizeof(sonFd))
            return -4;

//        printf("sonFd = %d\n",sonFd);
        int whileNum = 20;
        memset(&g_regsOffset,0,sizeof(struct regs_struct_offset));
        while(--whileNum)
        {
            wait(0);
            memset(&regs,0,sizeof(regs));
            ptrace(PTRACE_GETREGS, pid, 0, &regs);

            long *pRegs = (long*)&regs;
            for(int i=0;i<regsNum;++i)
            {
                // 命中返回值
                if(g_regsOffset.ret == 0
                    && pRegs[i] == -38) g_regsOffset.ret = i;
                else
                {
                    // 命中第一个参数
                    if(g_regsOffset.argv1 == 0
                        && pRegs[i] == sonFd) g_regsOffset.argv1 = i;
                    // 命中第二个参数
                    if(g_regsOffset.argv2 == 0)
                    {
                        long tmp = ptrace(PTRACE_PEEKDATA, pid, pRegs[i], NULL);
                        if(!memcmp(&tmp,msg,sizeof(long))
                            || !memcmp(&tmp,msg2,sizeof(long)))
                            g_regsOffset.argv2 = i;
                    }
                    // 命中第三个参数
                    if(g_regsOffset.argv3 == 0
                        && (pRegs[i] == msgLen
                            || pRegs[i] == msgLen2))
                    {
                        g_regsOffset.argv3 = i;
                    }
                }
            }

            if(g_regsOffset.ret != 0 && g_regsOffset.argv1 != 0
                && g_regsOffset.argv2 != 0 && g_regsOffset.argv3 != 0)
            {
                for(int j=0;j<regsNum;++j)
                {
                    // 命中系统调用号
                    if(j != g_regsOffset.ret && j != g_regsOffset.argv1
                        && j != g_regsOffset.argv2 && j != g_regsOffset.argv3)
                    {
                        // write调用号 在有些内核等于 4 有些内核等于 1
                        if(pRegs[j] == 1 || pRegs[j] == 4)
                        {
                            g_regsOffset.call = j;
                            break;
                        }
                    }
                }
            }

            if(ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1 ) {
                perror("ptrace syscall");
                ret = -7;
            }
            if(g_regsOffset.call)
            {
                ret = 0;
                break;
            }
        }
        ptrace(PTRACE_DETACH, pid, 0, 0);
    }
    else
    {
        perror("initRegsOffset fork");
    }
    if(pr >= 0) close(pr);
    if(pw >= 0) close(pw);
    return ret;
}
