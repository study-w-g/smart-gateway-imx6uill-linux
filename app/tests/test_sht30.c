#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

struct sht30_measurement {
	uint16_t raw_temperature;
	uint16_t raw_humidity;
};

int main(void)
{
	struct sht30_measurement measurement;
	int fd = open("/dev/sht30", O_RDONLY);
	ssize_t length;

	if (fd < 0) {
		perror("打开 /dev/sht30 失败");
		return 1;
	}
	length = read(fd, &measurement, sizeof(measurement));
	close(fd);
	if (length != sizeof(measurement)) {
		perror("读取 SHT30 失败");
		return 1;
	}
	printf("SHT30 原始温度=%u，温度=%.2f ℃，原始湿度=%u，湿度=%.2f %%\n",
	       measurement.raw_temperature,
	       -45.0 + 175.0 * measurement.raw_temperature / 65535.0,
	       measurement.raw_humidity,
	       100.0 * measurement.raw_humidity / 65535.0);
	return 0;
}
