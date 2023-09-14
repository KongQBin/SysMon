#include "callbacks.h"
#include <pthread.h>
#include "general.h"
#include "defunc.h"

struct rb_root *cbTree = NULL;
pid_t contpid = 0;
pthread_t thread_id;

void printArgv(pid_t child, struct user_regs_struct *reg)
{
    long temp_long;
    char message[1000] = {0};
    char* temp_char2 = message;
    for(int i=0;i<reg->rdx/WORDLEN+1;++i)
    {
        temp_long = ptrace(PTRACE_PEEKDATA, child, reg->rsi + (i*WORDLEN) , NULL);
        memcpy(temp_char2, &temp_long, WORDLEN);
        temp_char2 += WORDLEN;
    }
    message[reg->rdx] = '\0';
    dmsg(">>>>>>>>>>    %s\n",message);
    if(reg->rax != -38 && strstr(message,"123"))
    {
        reg->rax = 6;
        dmsg("mod rax\n");
    }
    else if(reg->rax == -38 && strstr(message,"123"))
    {
        reg->rdi = 1;
        dmsg("mod rid\n");
    }
     ptrace(PTRACE_SETREGS, child, NULL, reg);
}

void printUserRegsStruct(struct user_regs_struct *reg)
{
    // 被监控的系统调用的参数
//    dmsg("First argument: %llu\n", reg->rdi);
//    dmsg("Second argument: %llu\n", reg->rsi);
//    dmsg("Third argument: %llu\n", reg->rdx);


     unsigned long long int* regs = (unsigned long long int*)reg;

//    if(regs[3] == reg->r12)         dmsg("AAAAAAAAAAA\n");
//    if(regs[4] == reg->rbp)         dmsg("BBBBBBBBBBB\n");
//    if(regs[5] == reg->rbx)         dmsg("CCCCCCCCCCC\n");
//    if(regs[8] == reg->r9)          dmsg("DDDDDDDDDDD\n");
//    if(regs[10] == reg->rax)        dmsg("EEEEEEEEEEE\n");
//    if(regs[15] == reg->orig_rax)   dmsg("FFFFFFFFFFF\n");
//    if(regs[18] == reg->eflags)     dmsg("GGGGGGGGGGG\n");
//    if(regs[20] == reg->ss)         dmsg("HHHHHHHHHHH\n");
     for(int i=0;i<sizeof(struct user_regs_struct)/sizeof(unsigned long long int);++i)
         dmsg("%lld\t",regs[i]);
     dmsg("\n");
}

int monce = 1;
int monSysCall(struct rb_root *cbTree,pid_t child)
{
     struct user_regs_struct reg;
     memset(&reg,0,sizeof(reg));
     // 获取子进程寄存器的值
     ptrace(PTRACE_GETREGS, child, 0, &reg);

     long *pregs = (long*)&reg;
     //    printUserRegsStruct(&reg);
//     dmsg("Call %d\n",CALL(pregs));
     // 目标进程退出
     if(CALL(pregs) == ID_EXIT_GROUP) return -1;

     struct syscall *call = searchCallbackTree(cbTree,CALL(pregs));
     if(!call)
     {
//         dmsg("CALL(pregs):%d doesn't exist in callback tree!\n",CALL(pregs));
         return -2;
     }

     if(monce && CALL(pregs) == ID_EXECVE && IS_BEGIN(pregs))
     {
         -- monce;
         printf("------------------3\n");
         return -3;
     }

     dmsg("Call %d\n",CALL(pregs));
     IS_BEGIN(pregs) && !(CALL(pregs)&dos()) ?
         ((long (*)(pid_t,long *))call->cbf)(child,pregs) :
         ((long (*)(pid_t,long *))call->cef)(child,pregs);
     return CALL(pregs);
}

struct threadArgs
{
     pid_t pid;
     struct rb_root *cbTree;
};

// ptrace使用了PTRACE_O_TRACEFORK就自动附加了子进程了，无法拉起子线程再进行附加了，没办法并发
/* 解决思路一：while(wait)仅负责遍历进程及其子进程事件，
 * SysMon
 * while（1）                                                         JYNZDFY
 * wait（pid） <----------------------------------、
 * create thread                 ---------ASYNC---:-------->      分析事件并进行工作
 * continue等待下一个(父进程/子进程)事件          :                       |
 *                                                :                       V
 *                                                :      根据扫描结果决定是否允许ptrace对其放行
 *                                                :                       |
 *                                                :                       V
 *                                                `---------------   通知SysMon
 *
 */

/**/
enum ANALYSISRET
{
    A_SUCC = 0,
    A_EXIT = 1,
};

inline void printProcId(pid_t *pid,int *status)
{
     pid_t spid;
     //            // 获取子进程的PID
     if(ptrace(PTRACE_GETEVENTMSG, *pid, NULL, &spid) >= 0)
         dmsg("Child process created: %d status = %d\n", spid,*status);
     else
         dmsg("PTRACE_GETEVENTMSG : %s(%d) pid is %d\n", strerror(errno),errno,spid);
}
void printProcId(pid_t *pid,int *status);
enum ANALYSISRET analysis(pid_t *pid,int *status)
{
     // 设置下次监控的类型
     long opt = PTRACE_O_TRACESYSGOOD
                | PTRACE_O_TRACEEXEC
                | PTRACE_O_TRACEEXIT
                | PTRACE_O_TRACECLONE
                | PTRACE_O_TRACEFORK
                | PTRACE_O_TRACEVFORK;
     if(ptrace(PTRACE_SETOPTIONS, *pid, NULL, opt) < 0)
         dmsg("PTRACE_SETOPTIONS: %s(%d)\n", strerror(errno),*pid);


     //         dmsg(" waitpid is %d\n",targs->pid);
     long sig = 0,event = 0;

     sig = WSTOPSIG(*status);
     switch (sig) {
     case SIGTERM:  //kill -15
     case SIGINT:   //Ctrl + c
         if (ptrace(PTRACE_CONT, *pid, NULL, sig) < 0)
             dmsg("PTRACE_CONT : %s(%d) pid is %d\n", strerror(errno),errno,*pid);
         return A_SUCC;
         break;
     case SIGCONT:
//         printf("get signal is SIGCONT\n");
//         if(ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0)
//         {
//             dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
//             if(errno == 3) return errno;   //No such process
//         }
         break;
     default:
//         dmsg("Unknown signal is %d\n",sig);
         break;
     }


     event = (*status >> 16);
//     dmsg("status = %d\tsig = %lld\tevent=%lld\n",*status,sig,event);
     MANAGE_EVENT(event);
     if(event)
     {
         if(ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0)
         {
             dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
             if(errno == 3) return errno;   //No such process
         }
         return A_SUCC;
     }

     int monRet = monSysCall(cbTree,*pid);
     if(monRet != -2) printf("monRet = %d\n",monRet);
     if(monRet == -1) printf("stopMon pid is %d\n", *pid);
     if(monRet != -3)
     {
         if(ptrace(PTRACE_SYSCALL, *pid, 0, 0) < 0)
         {
             dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,*pid);
             if(errno == 3) return errno;   //No such process
         }
     }
     else
     {
         contpid = *pid;
     }
     return 0;
}

int controls(pid_t *pid,int *status)
{
    // 检查 容器 遍历已经处理完成的事件 然后放行


     // PTRACE_SYSEMU使得pid进程暂停在每次系统调用入口处。
     //    if (ptrace(PTRACE_SYSEMU, mtargs->pid, 0, 0) < 0) {
     //        dmsg("PTRACE_SYSEMU : %s(%d) pid is %d\n",strerror(errno),errno,mtargs->pid);
     //        //        return NULL;
     //    }
     return 0;
}

void signalHandler(int signum) {
     if (signum == SIGUSR1) {
         printf("pthread_t = %llu get signal is SIGUSR1 cont pid is %d\n",gettid(),contpid);
         if(ptrace(PTRACE_SYSCALL, contpid, 0, 0) < 0)
             dmsg("PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,contpid);
     }
}

void registerSignal()
{
     printf("pthread_t = %llu\n",gettid());
     // 注册信号处理函数
     struct sigaction sa;
     sa.sa_handler = signalHandler;
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = 0;
     sigaction(SIGUSR1, &sa, NULL);
}

void* startMon(void* ppid)
{
     registerSignal();

     pid_t pid = *(pid_t*)ppid;
     int status,ret;
     int toControls;
     dmsg("startMon pid is %d\n",pid);
     // 附加到被传入PID的进程
     ret = ptrace(PTRACE_ATTACH, pid, 0, 0);
     if(ret)
     {
         dmsg("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,pid);
         return NULL;
     }

     while (1)
     {
         status = 0;
         toControls = 1;
         pid = wait4(-1,&status,/*WNOHANG|*/WUNTRACED,0);
         if(pid && status)
         {
             enum ANALYSISRET ret = analysis(&pid,&status);        // 分析
             switch (ret) {
             case A_SUCC:
                 toControls = 1;
                 break;
             case A_EXIT:
                 toControls = 0;
                 break;
             default:
                 dmsg("Unknown ANALYSISRET = %d\n",ret);
                 break;
             }
             if(toControls) controls(&pid,&status);        // 处理
         }
     }

     // DETACH注销我们的跟踪,target process恢复运行
     ptrace(PTRACE_DETACH, pid, 0, 0);
     //    unInit(targs->cbTree);
     return 0;
}

int createMonThread(pid_t pid)
{
     pid_t *ppid = (pid_t*)calloc(1,sizeof(pid_t));
     if(!ppid) return -1;
     *ppid = pid;

    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, startMon, (void*)ppid);
    pthread_attr_destroy(&attr);

    return 0;
}
int main(int argc, char** argv)
{
    signal(SIGUSR1,SIG_IGN);
    int ret = init();
    cbTree = (struct rb_root*)calloc(1,sizeof(struct rb_root));
    if(!cbTree) {return -1;}
    if(!ret) ret = insertCallbackTree(cbTree,ID_WRITE,cbWrite,ceWrite);
    if(!ret) ret = insertCallbackTree(cbTree,ID_FORK,cbFork,ceFork);
    if(!ret) ret = insertCallbackTree(cbTree,ID_CLONE,cbClone,ceClone);
    if(!ret) ret = insertCallbackTree(cbTree,ID_EXECVE,cbExecve,ceExecve);

    if(!ret) createMonThread(atoi(argv[1]));
    sleep(10);
    pthread_kill(thread_id,SIGUSR1);
    while(1) sleep(100);
    return ret;
}
