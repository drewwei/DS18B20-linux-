#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

struct temp {
	int temp_l;
	int temp_h;
};

int main(int argc, char ** argv)
{
	int fd ;
	int ret;
	fd = open("/dev/ds18b20", O_RDWR);
	struct temp tem;
	int i ,j;
	while(1)
	{
		ret = read(fd ,&tem ,sizeof(tem));
		if(0 != ret)
		{
			printf("read erro!\n");
			return -1;
		}
		printf("current temperature:%d.%04d\n",tem.temp_h,tem.temp_l);
		for(i = 0; i < 10000; i++)
			for(j = 0; j < 5000; j++);
	}	
	return 0;
}