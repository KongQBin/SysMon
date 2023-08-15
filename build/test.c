#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
int main()
{
	int i = 99999;
	printf("%d\n",getpid());
	time_t begin, end;
	while(--i)
	{
		//time(&begin);
		sleep(3);
		int fd = open("/home/user/test.txt",O_RDWR|O_CREAT,0600);
		if(fd < 0)
		{
			printf("open err : %s\n",strerror(errno));
			break;
		}
		int ret = write(fd,"12345",strlen("12345"));
		printf("%d\n",ret);
		close(fd);
		//time(&end);
		//printf("while time = %lld\n",end - begin);
	}
	return 0;
}
