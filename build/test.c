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
        close(fd);
	}
	return 0;
}
