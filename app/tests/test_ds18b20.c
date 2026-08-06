#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/* 独立测试程序只验证驱动接口，不包含完整网关业务。 */
int main(void)
{
	int16_t raw;
	int fd = open("/dev/ds18b20", O_RDONLY);
	ssize_t length;

	if (fd < 0) {
		perror("打开 /dev/ds18b20 失败");
		return 1;
	}
	length = read(fd, &raw, sizeof(raw));
	close(fd);
	if (length != sizeof(raw)) {
		perror("读取 DS18B20 失败");
		return 1;
	}
	printf("DS18B20 原始值=%d，摄氏温度=%.2f\n", raw,
	       (double)raw / 16.0);
	return 0;
}
