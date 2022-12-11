/*!
 * @file app_beep.c
 * @brief beep测试
 * @par Copyright(c):    Revised BSD License,see section LICENSE
 * @author shichao ()
 * @date 2022-12-11
 * @version 1.0.0
 * @par History:
 * version | author | date | desc
 * --- | --- | --- | ---
 * V1.0.0 | shichao | 2022-12-11 | 创建第一版
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int fd = 0;
    int iret = 0;
    int data = 0;
	unsigned char databuf[1] = {0};

	fd = open("/dev/beep", O_RDWR);
	if(fd < 0)
    {
		printf("open /dev/beep failed!\n");
		return -1;
	}

    while(1)
    {
        printf("please input 0 or 1, input other exit:");
        scanf("%1d", &data);
        if(0 != data && 1 != data)
        {
            break;
        }
        memset(databuf, 0x00, sizeof(databuf));
        databuf[0] = data;
        iret = write(fd, databuf, sizeof(databuf));
        if(iret < 0)
        {
            printf("LED Control Failed!\n");
        }
    }
	close(fd);
	return 0;
}
