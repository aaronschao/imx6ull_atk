/*!
 * @file app_timer.c
 * @brief timer测试
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
#include <linux/ioctl.h>

#define OPEN_CMD            (_IO(0XEF, 0x1))              /* 打开定时器 */
#define CLOSE_CMD           (_IO(0XEF, 0x2))              /* 关闭定时器 */
#define SETPERIOD_CMD       (_IO(0XEF, 0x3))              /* 设置定时器周期命令 */

int main(int argc, char *argv[])
{
	int fd = 0;
    int data = 0;
	int period = 0;
    int cmd[] = {CLOSE_CMD, OPEN_CMD, SETPERIOD_CMD};

	fd = open("/dev/timer", O_RDWR);
	if(fd < 0)
    {
		printf("open /dev/beep failed!\n");
		return -1;
	}

    while(1)
    {
        printf("please input 0 1 2, input other exit:");
        scanf("%1d", &data);
        if(0 != data && 1 != data && 2 != data)
        {
            break;
        }

        if(2 == data)
        {
            printf("please input timer period:");
            scanf("%d", &period);
        }
        ioctl(fd, cmd[data], period);
    }
	close(fd);
	return 0;
}
