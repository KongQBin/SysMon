#include "sigevt.h"
#define RETURN(tp)    ({*type = tp; return;})
void sigEvt(PidInfo *pinfo, TASKTYPE *type)
{
//    DMSG(ML_INFO,"pid is %llu status = %d\n",pinfo->pid,pinfo->status);
    if(WIFSIGNALED(pinfo->status) || WIFEXITED(pinfo->status))/*由信号导致退出或者正常退出*/
    {
        if(WIFEXITED(pinfo->status))
            DMSG(ML_INFO,"process %llu exit\n",pinfo->pid);
        else
            DMSG(ML_INFO,"process %llu exit signal is %d\n",pinfo->pid,WTERMSIG(pinfo->status));
//        RETURN(TT_TARGET_PROCESS_EXIT);
    }

    int event = pinfo->status >> 16;
    int signal = WSTOPSIG(pinfo->status);
    DMSG(ML_INFO,">> process %llu status = %d event is %d signal is %d\n",pinfo->pid,pinfo->status,event,signal);
    switch (event){
    // 0 证明这是单纯的信号
    case 0:
        if(signal == TRAP_SIG)
            RETURN(TT_IS_SYSCALL);
        else // attach模式没办法做到STOP
        {
//            siginfo_t tmp;
//            int stopped = ptrace(PTRACE_GETSIGINFO, pinfo->pid, 0, &tmp) < 0;
//            DMSG(ML_INFO,"stopped = %d\n",stopped);
            if(signal == SIGCONT) UNSET_STOP(pinfo->flags);
            pinfo->status = signal;
            RETURN(TT_IS_SIGNAL);
        }
        break;
    case PTRACE_EVENT_STOP:
        switch (signal) {
        case SIGSTOP:
        case SIGTSTP:
        case SIGTTIN:
        case SIGTTOU:
        {
            DMSG(ML_INFO,"PTRACE_EVENT_STOP process %llu signal is %d\n",pinfo->pid,signal);
//            pinfo->status = signal;
            SET_STOP(pinfo->flags);
            RETURN(TT_IS_SIGNAL_STOP);
        }
            break;
        default:
            RETURN(TT_SUCC);
        }
    case PTRACE_EVENT_EXIT:
        RETURN(TT_TARGET_PROCESS_EXIT);
    default:
        RETURN(TT_IS_EVENT);
    }
}
