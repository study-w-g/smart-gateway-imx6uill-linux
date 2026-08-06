#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../../drivers/gpio_event/gpio_event_uapi.h"

int main(void)
{
	struct gpio_event_record event;
	uint32_t led = 0;
	int fd = open("/dev/gpio-event", O_RDWR);
	ssize_t length;

	if (fd < 0) {
		perror("打开 /dev/gpio-event 失败");
		return 1;
	}
	if (ioctl(fd, GPIO_EVENT_IOC_SET_LED, &led) < 0)
		perror("关闭 LED 失败");
	printf("等待按键事件，按 Ctrl+C 退出。\n");
	while ((length = read(fd, &event, sizeof(event))) == sizeof(event))
		printf("事件：电平=%u，序号=%u，时间戳=%llu\n", event.value,
		       event.sequence, (unsigned long long)event.timestamp_ns);
	close(fd);
	return length < 0 ? 1 : 0;
}
