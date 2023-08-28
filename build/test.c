#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
int main()
{
    printf("%d\n",getpid());

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
	return 0;
}
