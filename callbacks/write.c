#include "callbacks.h"

long cbWrite(pid_t pid, long *regs)
{
    long ret = 0;
    long temp_long;
    char message[128] = {0};
    char *test = "abcd\n";
    long newMsgLen = 0;
    for(int i=0;i<ARGV_3(regs)/WORDLEN+1;++i)
    {
        temp_long = ptrace(PTRACE_PEEKDATA, pid, ARGV_2(regs) + (i*WORDLEN), NULL);
        memcpy(&message[i*WORDLEN], &temp_long, WORDLEN);
    }
    newMsgLen = strlen(test) > ARGV_3(regs) ? ARGV_3(regs) : strlen(test);

    if(!memcmp(message,"12345",ARGV_3(regs))
        || !memcmp(message,test,ARGV_3(regs)))
    {
        printf("ARGV_3(regs) = %d\n",ARGV_3(regs));
        printf("message = ");
        for(int i=0;i<ARGV_3(regs);++i) printf("%c",message[i]);
        printf("\n");

        // 修改write的参数1为1
        // 也就是将文件描述符修改为标准输出
        ARGV_1(regs) = 1;
        ARGV_3(regs) = newMsgLen;

        ret = ptrace(PTRACE_SETREGS, pid, NULL, regs);
        if(ret < 0) printf("%s\n",strerror(errno));
        if(!memcmp(message,test,ARGV_3(regs))) {printf("return 0\n"); return 0;}

        long tmp;
        memcpy(message,test,newMsgLen);
        char *ctmp = message;
        for(int i=0; i<ARGV_3(regs)/WORDLEN + 1; ++i)
        {
            tmp = 0;
            ctmp += (i*WORDLEN);
            memcpy(&tmp,ctmp,WORDLEN);
            for(int j=0;j<WORDLEN;++j) printf("0x%x\t",((char*)&tmp)[j]);
            printf("\n");
            ret = ptrace(PTRACE_POKEDATA, pid, ARGV_2(regs) + (i*WORDLEN), tmp);
            if(ret < 0) printf("%s\n",strerror(errno));
        }
    }
    return 0;
}

long ceWrite(pid_t pid, long *regs)
{
    long temp_long;
    char message[1000] = {0};
    char* temp_char2 = message;
    for(int i=0;i<ARGV_3(regs)/WORDLEN+1;++i)
    {
        temp_long = 0;
        temp_long = ptrace(PTRACE_PEEKDATA, pid, ARGV_2(regs) + (i*WORDLEN) , NULL);
        memcpy(temp_char2, &temp_long, WORDLEN);
        temp_char2 += WORDLEN;
    }
    if(strstr(message,"abcde"))
    {
        printf("E: ARGV_3(regs) = %d\n",ARGV_3(regs));
        //    message[ARGV_3(regs)] = '\0';
        printf("EE: ");
        for(int i=0;i<WORDLEN;++i) printf("0x%x\t",message[i]);
        printf("\nwrite(%d,\"%s\")\n",ARGV_1(regs),message);
        //    // 修改返回值(可以根据已获取到的条件)
        //    regs->rax = 6;
        //    ptrace(PTRACE_SETREGS, pid, NULL, regs);
    }
    return 0;
}
