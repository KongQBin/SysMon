#include "taskopt.h"
void taskOpt(ManageInfo *minfo,ControlBaseInfo *cbinfo)
{
    do{
        if(!gDefaultControlPolicy)   break;
        switch (minfo->type) {
        case MT_Init:
            memcpy(&gDefaultControlPolicy->binfo,cbinfo,sizeof(ControlBaseInfo));
            break;
        case MT_AddTid:
            DMSG(ML_INFO,"MT_AddTid %llu\n",minfo->tpid);
            if(!ptraceAttach(minfo->tpid))
                pidInsert(&gPidTree,createPidInfo(minfo->tpid,0,0));
            break;
        case MT_CallPass:
//            DMSG(ML_INFO,"MT_CallPass %llu\n",minfo->tpid);
            if(ptrace(PTRACE_SYSCALL, minfo->tpid, 0, 0) < 0)
                DMSG(ML_WARN,"PTRACE_SYSCALL : %s(%d) pid is %d\n",strerror(errno),errno,minfo->tpid);
            delPinfo(minfo->tpid,-1,0);
            break;
        case MT_ToExit:
        {
            DMSG(ML_INFO,"MT_ToExit TreeSize = %llu\n",pidTreeSize(&gPidTree));
//            sleep(1);
//            break;
            //            break;

            // 有待再次验证
            // 此处由于是强制退出，期间要发送Stop信号向被监控的进程
            // 所以发送STOP信号后最好不要有任何打印信息，因为桌面进程也在其中，比如gnome

            FOR_TREE(node,gPidTree)
            {
                pid_t detachid = getStruct(node)->pid;
//                DMSG(ML_INFO,"detachid = %llu\n",detachid);
                // 先用正常手段解除监控
                if(ptraceDetach(detachid,0,1))
                {
                    // 追踪解除失败,查看进程是否还存在
                    if(tkill(detachid,0) && errno == ESRCH)
                    {
                        // 进程不存在，删除这个进程
                        pidDelete(&gPidTree,detachid);
                        resetItNode();
                        DERR(tkill);
                    }
                    else
                    {
                        // 进程存在，SEIZE模式 ？ 触发PTRACE_INTERRUPT ： 发送SIGSTOP
                        if(gSeize ? ptrace(PTRACE_INTERRUPT, detachid, 0, 0) < 0 : tkill(detachid,SIGSTOP))
                            DERR(PTRACE_INTERRUPT);
                        else
                        {
                            // 接收由于PTRACE_INTERRUPT/STOP信号触发的事件
                            for(;;)
                            {
                                int status = 0, ret = 0, succ = 0;
                                ret = waitpid(detachid, &status, __WALL);
                                if(ret < 0)
                                //                                if(wait4(detachid, &status,WUNTRACED,__WALL) < 0)
                                {
                                    if (errno == EINTR)
                                        continue;
                                    DERR(waitpid);
                                    DMSG(ML_WARN,"ret = %d\n",ret);
                                    break;
                                }
                                int sig = WSTOPSIG(status);
                                int evt = (unsigned)status >> 16;
                                if(gSeize)
                                {
                                    if(evt == PTRACE_EVENT_STOP || sig == TRAP_SIG)
                                        sig = 0;
                                    succ = !ptraceDetach(detachid,sig);
                                }
                                else
                                {
                                    if(sig == SIGSTOP)
                                        succ = !ptraceDetach(detachid);
                                }

                                // 解除成功
                                if(succ)
                                {
                                    DMSG(ML_INFO,"Ptrace force detach %llu success... \n",detachid);
                                    pidDelete(&gPidTree,detachid);
                                    resetItNode();
                                    break;
                                }
                                // 不满足解除条件，继续这个进程，进入下一次循环等待
                                else
                                {
                                    if(ptrace(PTRACE_CONT, detachid, 0, sig) < 0)
                                        DMSG(ML_WARN,"PTRACE_CONT: %s(%d)\n", strerror(errno),detachid);
                                }
                            }
                        }
                    }
                }
                else
                {
                    pidDelete(&gPidTree,detachid);
                    resetItNode();
                }
            }
            // 依然解除失败
            FOR_TREE(node,gPidTree)
            {
                pid_t detachid = getStruct(node)->pid;
                DMSG(ML_ERR,"process %llu detach fail!\n",detachid);
            }
            break;
        }
        default:
            break;
        }
    }while(0);
}
