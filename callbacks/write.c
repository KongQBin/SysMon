#include "write.h"

long writeCallBegin(pid_t pid, long *regs)
{
    long temp_long;
    char message[1000] = {0};
    char* temp_char2 = message;
    for(int i=0;i<ARGV_3(regs)/WORDLEN+1;++i)
    {
        temp_long = ptrace(PTRACE_PEEKDATA, pid, ARGV_2(regs) + (i*WORDLEN), NULL);
        memcpy(temp_char2, &temp_long, WORDLEN);
        temp_char2 += WORDLEN;
    }
    message[ARGV_3(regs)] = '\0';

    char *test = "abcdefghijklmn";
    char *ctmp = test;
    if(!strcmp(message,"12345") || strstr(message,"abcde"))
    {
        printf("ARGV_3(regs) = %d\n",ARGV_3(regs));
        printf("message = %s\n",message);
        printf("A: ");
        for(int i=0;i<strlen(message)+1;++i)
        {
            printf("0x%x\t",((char*)&message)[i]);
        }
        printf("\n");

        // 修改write的参数1为1
        // 也就是将文件描述符修改为标准输出
        ARGV_1(regs) = 1;
        ARGV_3(regs) = strlen(test)+1;
        printf("S: ARGV_1(regs) = %d\n",ARGV_1(regs));
        printf("S: ARGV_3(regs) = %d\n",ARGV_3(regs));
        long ret = ptrace(PTRACE_SETREGS, pid, NULL, regs);
        if(ret < 0)
            printf("%s\n",strerror(errno));
        if(strstr(message,"abcde")) return 0;

        long tmp;
        for(int i=0; i<strlen(test)/WORDLEN + 1; ++i)
        {
            printf("B: ");
            tmp = 0;
            ctmp += (i*WORDLEN);
            memcpy(&tmp,ctmp,strlen(ctmp) < WORDLEN ? strlen(ctmp) : WORDLEN);
            for(int j=0;j<WORDLEN;++j)
            {
                printf("0x%x\t",((char*)&tmp)[j]);
            }
            printf("\n");
            long ret = ptrace(PTRACE_POKEDATA, pid, ARGV_2(regs) + (i*WORDLEN), tmp);
            printf("ptrace ret: %d\n",ret);
            if(ret < 0) printf("%s\n",strerror(errno));
        }


        //        {
        //            long temp_long;
        //            char message[1000] = {0};
        //            char* temp_char2 = message;
        //            for(int i=0;i<ARGV_3(regs)/WORDLEN+1;++i)
        //            {
        //                temp_long = ptrace(PTRACE_PEEKDATA, pid, ARGV_2(regs) + (i*WORDLEN), NULL);
        //                memcpy(temp_char2, &temp_long, WORDLEN);
        //                temp_char2 += WORDLEN;
        //            }
        //            message[ARGV_3(regs)] = '\0';

        //            printf("C: ");
        //            for(int j = 0; j<sizeof(temp_long); ++j)
        //            {
        //                printf("0x%x ",((char*)&temp_long)[j]);
        //            }
        //            printf("\nmessage2 = %s\n",message);
        //        }
    }

    return 0;
}

long writeCallEnd(pid_t pid, long *regs)
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
    if(strstr(message,"abcdefg"))
    {
        printf("E: ARGV_3(regs) = %d\n",ARGV_3(regs));
        //    message[ARGV_3(regs)] = '\0';
        printf("EE: ");
        for(int i=0;i<WORDLEN;++i)
            printf("0x%x\t",message[i]);
        printf("\nwrite(%d,\"%s\")\n",ARGV_1(regs),message);
        printf("\n\n\n");

        //    // 修改返回值(可以根据已获取到的条件)
        //    regs->rax = 6;
        //    ptrace(PTRACE_SETREGS, pid, NULL, regs);
    }
    return 0;
}
