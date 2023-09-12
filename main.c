#include "callbacks.h"
#include <pthread.h>

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
    printf(">>>>>>>>>>    %s\n",message);
    if(reg->rax != -38 && strstr(message,"123"))
    {
        reg->rax = 6;
        printf("mod rax\n");
    }
    else if(reg->rax == -38 && strstr(message,"123"))
    {
        reg->rdi = 1;
        printf("mod rid\n");
    }
     ptrace(PTRACE_SETREGS, child, NULL, reg);
}

void printUserRegsStruct(struct user_regs_struct *reg)
{
    // 被监控的系统调用的参数
//    printf("First argument: %llu\n", reg->rdi);
//    printf("Second argument: %llu\n", reg->rsi);
//    printf("Third argument: %llu\n", reg->rdx);


     unsigned long long int* regs = (unsigned long long int*)reg;

//    if(regs[3] == reg->r12)         printf("AAAAAAAAAAA\n");
//    if(regs[4] == reg->rbp)         printf("BBBBBBBBBBB\n");
//    if(regs[5] == reg->rbx)         printf("CCCCCCCCCCC\n");
//    if(regs[8] == reg->r9)          printf("DDDDDDDDDDD\n");
//    if(regs[10] == reg->rax)        printf("EEEEEEEEEEE\n");
//    if(regs[15] == reg->orig_rax)   printf("FFFFFFFFFFF\n");
//    if(regs[18] == reg->eflags)     printf("GGGGGGGGGGG\n");
//    if(regs[20] == reg->ss)         printf("HHHHHHHHHHH\n");
     for(int i=0;i<sizeof(struct user_regs_struct)/sizeof(unsigned long long int);++i)
         printf("%lld\t",regs[i]);
     printf("\n");
}

int monSysCall(struct rb_root *cbTree,pid_t child)
{
     struct user_regs_struct reg;
     memset(&reg,0,sizeof(reg));
     // 获取子进程寄存器的值
     ptrace(PTRACE_GETREGS, child, 0, &reg);

     long *pregs = (long*)&reg;
     //    printUserRegsStruct(&reg);
//     printf("Call %d\n",CALL(pregs));
     // 目标进程退出
     if(CALL(pregs) == ID_EXIT_GROUP) return -1;

     printf("[%d]\tCall %d\n",gettid(),CALL(pregs));
     struct syscall *call = searchCallbackTree(cbTree,CALL(pregs));
     if(!call)
     {
//         printf("[%d]\tCALL(pregs):%d doesn't exist in callback tree!\n",gettid(),CALL(pregs));
         return 0;
     }
     IS_BEGIN(pregs) && !(CALL(pregs)&dos()) ?
         ((long (*)(pid_t,long *))call->cbf)(child,pregs) :
         ((long (*)(pid_t,long *))call->cef)(child,pregs);
}

struct threadArgs
{
     pid_t pid;
     struct rb_root *cbTree;
};

// ptrace使用了PTRACE_O_TRACEFORK就自动附加了子进程了，无法拉起子线程再进行附加了，没办法并发
/* 解决思路一：while(wait)仅负责遍历进程及其子进程事件，
 * while（1）
 * wait（pid）
 * create thread                 ----ASYNC--->             分析事件并进行工作
 * continue等待下一个(父进程/子进程)事件                           |
 *                                                                 V
 *                                              根据进程ID利用ptrace重新允许目标ID向下执行
 */

/* 解决思路二：依旧监听到子进程的创建立即新建子线程，子进程等待主线程通过使用PTRACE_DETACH移除对目标进程的子进程的附加，转而由子线程附加
 * if(PTRACE_EVENT_FORK...)
 *      create thread       ----ASYNC--->       等待目标进程被主线程解除附加
 *      解除对新进程的附加                                   |
 *      继续...                                              V
 *                                                      附加目标进程
 *                                                           |
 *                                                           V
 *                                                      进入wait循环
 *                                                           |
 *                                                           V
 *                                                 if(PTRACE_EVENT_FORK...)
 *                                                           |
 *                                                           V
 *                                                     create thread       ----ASYNC--->       等待目标进程被主线程解除附加
 *                                                           |                                             |
 *                                                           V                                             V
 *                                                   解除对新进程的附加                               附加目标进程
 *                                                           |                                             |
 *                                                           V                                             V
 *                                                        继续...                                         ...
 *
 *
 * 经测试：思路二依旧存在漏事件的问题（比在callbacks的clone中创建线程漏的少一些）
 * 而且思路二无法做到主线程取消附加（PTRACE_DETACH）与子线程的附加（PTRACE_ATTACH）的无缝衔接
 * 以下是在目录中文件没有任何变化的情况下对同一个目录执行ls命令所过滤的事件数量，几乎每次都不一样
 * [user@localhost SysMon]$ wc -l ./ddd*
 *   1373 ./ddd1
 *   1375 ./ddd2
 *   1387 ./ddd3
 *   1361 ./ddd4
 *   1389 ./ddd5
 *   1387 ./ddd6
 *   1375 ./ddd7
 *   1375 ./ddd8
 *  11022 总用量
 *
 * 故思路二暂时搁置，使用思路一进行开发测试
 */
int createMonThread(pid_t pid);
// 273 109
void* ptraceHook(void *args) {
     struct threadArgs *targs = (struct threadArgs *)args;
//void ptraceHook(struct rb_root *cbTree,pid_t child) {
    // 被监控的进程id
    printf("[%d]\tcalled by %d\n",gettid(), targs->pid);

    int status;
    long sig,event,opt = /*PTRACE_O_TRACESYSGOOD
                        | PTRACE_O_TRACEEXEC
                        | PTRACE_O_TRACEEXIT
                        |*/ PTRACE_O_TRACECLONE
                        | PTRACE_O_TRACEFORK
                        /*| PTRACE_O_TRACEVFORK*/;
    while (1)
    {
        // 等待子进程受到跟踪因为下一个系统调用而暂停
//         printf("[%d]\t waitpid S\n",gettid());
         status = 0;

         waitpid(targs->pid,&status,0);
//         printf("[%d]\t waitpid is %d\n",gettid(),targs->pid);

         //         pid_t tpid = wait4(-1,&status,0,0);
         if(ptrace(PTRACE_SETOPTIONS, targs->pid, NULL, opt) < 0)
             printf("[%d]\tPTRACE_SETOPTIONS: %s(%d)\n",gettid(), strerror(errno),targs->pid);

        sig = WSTOPSIG(status);
        event = (status >> 16);
//        printf("[%d]\tstatus = %d\tsig = %lld\tevent=%lld\n",gettid(),status,sig,event);
        if(event == PTRACE_EVENT_CLONE/*5*/
            || event == PTRACE_EVENT_FORK)
        {
            pid_t spid;
//            // 获取子进程的PID
            if(ptrace(PTRACE_GETEVENTMSG, targs->pid, NULL, &spid) >= 0)
            {
                printf("[%d]\tChild process created: %d status = %d\n",gettid(), spid,status);
                createMonThread(spid);
                // 暂停目标进程(记得在子线程中手动结束附加时发送SIGCONT信号继续运行
                // 因为此处发送STOP的情况下createMonThread创建出来的子线程结束附加后，目标进程会回到STOP状态
                kill(spid,SIGSTOP);
                sleep(0);
                if(ptrace(PTRACE_DETACH, spid, 0, 0) < 0)
                {
                    if(errno == 3)
                    {
                        int stat = 0;
                        waitpid(spid,&stat,0);
                        kill(spid,SIGSTOP);
                        if(ptrace(PTRACE_DETACH, spid, 0, 0) < 0)
                            printf("[%d]\tPTRACE_DETACH : %s(%d) pid is %d\n",gettid(), strerror(errno),errno,spid);
                    }
                }
//                sleep(1);
            }
            else
            {
                printf("[%d]\tPTRACE_GETEVENTMSG : %s(%d) pid is %d\n",gettid(), strerror(errno),errno,spid);
            }


            if (ptrace(PTRACE_CONT, targs->pid, NULL, sig) < 0) {
                printf("[%d]\tPTRACE_CONT : %s(%d) pid is %d\n",gettid(), strerror(errno),errno,targs->pid);
            }
            continue;
        }
        if (WIFSTOPPED(status))
        {

            // kill -15 || kill -2(ctrl + c)
            if(sig == SIGTERM
//                || sig == 17
                || sig == SIGINT)
            {
                if (ptrace(PTRACE_CONT, targs->pid, NULL, sig) < 0) {
                    printf("[%d]\tPTRACE_CONT : %s(%d) pid is %d\n",gettid(), strerror(errno),errno,targs->pid);
                }
//                if(sig != 17) continue;
                continue;
            }
        }
        if(monSysCall(targs->cbTree,targs->pid) == -1)
        {
            printf("[%d]\tstopMon pid is %d\n",gettid(), targs->pid);
            kill(targs->pid,SIGCONT);
            break;
        }
        if(ptrace(PTRACE_SYSCALL, targs->pid, 0, 0) < 0)
        {
            printf("[%d]\tPTRACE_SYSCALL : %s(%d) pid is %d\n",gettid(),strerror(errno),errno,targs->pid);
            if(errno == 3) break;   //No such process
        }
    }
    return NULL;
}


// PTRACE_SYSEMU使得pid进程暂停在每次系统调用入口处。
//    if (ptrace(PTRACE_SYSEMU, mtargs->pid, 0, 0) < 0) {
//        printf("PTRACE_SYSEMU : %s(%d) pid is %d\n",strerror(errno),errno,mtargs->pid);
//        //        return NULL;
//    }

void* startMon(void* targs)
{
    struct threadArgs *mtargs = (struct threadArgs *)targs;
    printf("[%d]\tstartMon pid is %d\n",gettid(),mtargs->pid);
    // 附加到被传入PID的进程
    int i = 10000;
    while(1)
    {
        int ret = ptrace(PTRACE_ATTACH, mtargs->pid, 0, 0);
        if(ret >= 0) break;
        else if( ret< 0)
        {
//            printf("[%d]\tPTRACE_ATTACH : %s(%d) pid is %d\n",gettid(),strerror(errno),errno,mtargs->pid);
//            usleep(0);
        }
    };
    if(!i)
    {
        kill(mtargs->pid,SIGCONT);
        printf("无法进行监控\n");
        return NULL;
    }
//    if (ptrace(PTRACE_ATTACH, mtargs->pid, 0, 0) < 0) {
//        printf("PTRACE_ATTACH : %s(%d) pid is %d\n",strerror(errno),errno,mtargs->pid);
////        return NULL;
//    }

    ptraceHook(targs);
    // DETACH注销我们的跟踪,target process恢复运行
    ptrace(PTRACE_DETACH, mtargs->pid, 0, 0);
//    unInit(targs->cbTree);
//    if(targs->cbTree)   free(targs->cbTree);
//    if(targs)           free(targs);
    return NULL;
}

int createMonThread(pid_t pid)
{
    struct threadArgs *targs = (struct threadArgs *)calloc(1,sizeof(struct threadArgs));
    if(!targs) return -1;
    targs->pid = pid;
    targs->cbTree = (struct rb_root*)calloc(1,sizeof(struct rb_root));
    if(!targs->cbTree) {free(targs);return -1;}
    int ret = 0;
    //    if(!ret) ret = insertCallbackTree(targs->cbTree,ID_WRITE,cbWrite,ceWrite);
    //    if(!ret) ret = insertCallbackTree(targs->cbTree,ID_FORK,cbFork,ceFork);
    if(!ret) ret = insertCallbackTree(targs->cbTree,ID_CLONE,cbClone,ceClone);
    //    if(!ret) ret = insertCallbackTree(targs->cbTree,ID_EXECVE,cbExecve,ceExecve);

    pthread_t thread_id;
    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, startMon, (void*)targs);
    pthread_attr_destroy(&attr);

    return 0;
}

int main(int argc, char** argv)
{
    int ret = init();
    if(!ret) createMonThread(atoi(argv[1]));
    sleep(10000);
    return ret;
}

// 放过发往子进程的信号
// 但经测，貌似不起作用
//     ptrace(PTRACE_SETOPTIONS, target_pid, NULL, PTRACE_O_TRACESYSGOOD);
