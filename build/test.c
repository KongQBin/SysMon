#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>


void testWrite()
{
    char *msg = (char*)calloc(1,5);
    memcpy(msg,"12345",5);
    int i = 999;
    while(--i)
    {
        sleep(2);
        int fd = open("/home/user/test.txt",O_RDWR|O_CREAT,0600);
        if(fd < 0)
        {
            printf("open err : %s\n",strerror(errno));
            break;
        }
        int ret = write(fd,msg,5);
        printf("1write ret is %d\n",3);
        if(ret < 0)
            printf("write err is %d\n",strerror(errno));
        {
            // 用于查看是否存在描述符泄漏
            printf("close fd is %d\n",fd);
            // 结果fd在监控进程write中被修改为1不影响当前fd的值
            // 原因是由于调用过程进行了值拷贝，监控进程修改的仅仅是被拷贝的那一份
            // 故不存在描述符泄漏的问题
        }
        close(fd);
    }
}

void testFork()
{
    sleep(8);
    pid_t pid = fork();
    if(pid == 0)
    {
        printf("this is son! pid is %d\n",getpid());
        int i = 10;
        while(--i) sleep(1);
        exit(0);
    }
    else if(pid > 0)
    {
        printf("this is parent! pid is %d\n",getpid());
        int i = 12;
        while(--i) sleep(1);
    }
    else
        printf("fork error : %s\n",strerror(errno));
}

int main()
{
    printf("%d\n",getpid());
    testFork();
	return 0;
}
