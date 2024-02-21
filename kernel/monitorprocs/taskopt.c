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
//            DMSG(ML_INFO,"MT_AddTid %llu\n",minfo->tpid);
            if(!ptraceAttach(minfo->tpid))
                pidInsert(&gPidTree,createPidInfo(minfo->tpid,0,0));
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
                if(ptraceDetach(detachid,1))
                {
                    // 追踪解除失败,查看进程是否还存在
                    if(syscall(__NR_tkill,detachid,0) && errno == ESRCH)
                    {
                        // 进程不存在，删除这个进程
                        pidDelete(&gPidTree,detachid);
                        resetItNode();
                        DERR(tkill);
                    }
                    else
                    {
                        // 进程存在，发送SIGSTOP
                        if(syscall(__NR_tkill,detachid,SIGSTOP))
                        //                        if(ptrace(PTRACE_INTERRUPT, detachid, 0, 0))
                        {
                            DERR(PTRACE_INTERRUPT);
                        }
                        else
                        {
                            // 接收由于STOP信号触发的事件
                            for(;;)
                            {
                                int status = 0;
                                int ret = waitpid(detachid, &status, __WALL);
                                if(ret < 0)
                                //                                if(wait4(detachid, &status,WUNTRACED,__WALL) < 0)
                                {
                                    if (errno == EINTR)
                                        continue;
                                    DERR(waitpid);
                                    DMSG(ML_WARN,"ret = %d\n",ret)
                                    break;
                                }
                                // 再次解除追踪
                                if(!ptraceDetach(detachid))
                                {
                                    syscall(__NR_tkill,detachid,SIGCONT);
                                    DMSG(ML_WARN,"Ptrace force detach %llu success... \n",detachid)
                                    pidDelete(&gPidTree,detachid);
                                    resetItNode();
                                    break;
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
                DMSG(ML_ERR,"process %llu detach fail!\n",detachid)
            }
            break;
        }
        default:
            break;
        }
    }while(0);
}
