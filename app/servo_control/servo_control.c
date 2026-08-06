#include "servo_control.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/*
 * LX-16A 指令帧格式：
 * 0x55 0x55 ID 指令 参数长度 参数 校验和
 *
 * 本文件把“串口配置”和“舵机协议帧”分开，便于学习：
 * - termios 负责 UART 的波特率、数据位、停止位和校验；
 * - 本文件负责 LX-16A 的命令字段和校验和。
 */
#define SERVO_HEADER 0x55
#define SERVO_MOVE_TIME_WRITE 1
#define SERVO_POS_READ 28

static speed_t baud_to_termios(int baudrate)
{
	switch (baudrate) {
	case 115200:
		return B115200;
	case 57600:
		return B57600;
	case 9600:
		return B9600;
	default:
		return 0;
	}
}

int servo_open(const char *device, int baudrate)
{
	struct termios options;
	speed_t speed = baud_to_termios(baudrate);
	int fd;

	if (!speed)
		return -EINVAL;
	fd = open(device, O_RDWR | O_NOCTTY | O_SYNC | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	if (tcgetattr(fd, &options) < 0)
		goto failed;

	cfmakeraw(&options);
	cfsetispeed(&options, speed);
	cfsetospeed(&options, speed);
	options.c_cflag |= CLOCAL | CREAD;
	options.c_cflag &= ~CSTOPB; /* 1 个停止位。 */
	options.c_cflag &= ~PARENB; /* 无校验位。 */
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;     /* 8 个数据位。 */
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 5;    /* 500 ms 读取超时。 */

	if (tcsetattr(fd, TCSANOW, &options) < 0)
		goto failed;
	tcflush(fd, TCIOFLUSH);
	return fd;

failed:
	close(fd);
	return -errno;
}

void servo_close(int fd)
{
	if (fd >= 0)
		close(fd);
}

static int servo_send_frame(int fd, uint8_t id, uint8_t command,
				    const uint8_t *params, uint8_t param_count)
{
	uint8_t frame[16];
	uint8_t length = param_count + 3;
	uint8_t checksum = id + length + command;
	ssize_t sent;
	int i;

	if (param_count > 10)
		return -EINVAL;
	frame[0] = SERVO_HEADER;
	frame[1] = SERVO_HEADER;
	frame[2] = id;
	frame[3] = length;
	frame[4] = command;
	for (i = 0; i < param_count; i++) {
		frame[5 + i] = params[i];
		checksum += params[i];
	}
	frame[5 + param_count] = (uint8_t)(~checksum);

	sent = write(fd, frame, 6 + param_count);
	if (sent != 6 + param_count)
		return sent < 0 ? -errno : -EIO;
	return 0;
}

int servo_move(int fd, uint8_t servo_id, uint16_t position,
	       uint16_t time_ms)
{
	uint8_t params[4] = {
		position & 0xff, position >> 8,
		time_ms & 0xff, time_ms >> 8,
	};

	if (position > 1000)
		return -EINVAL;
	return servo_send_frame(fd, servo_id, SERVO_MOVE_TIME_WRITE,
				params, sizeof(params));
}

int servo_read_position(int fd, uint8_t servo_id, uint16_t *position)
{
	uint8_t response[8];
	ssize_t length;
	uint8_t checksum;

	if (!position)
		return -EINVAL;
	if (servo_send_frame(fd, servo_id, SERVO_POS_READ, NULL, 0) < 0)
		return -EIO;
	length = read(fd, response, sizeof(response));
	if (length < 8)
		return length < 0 ? -errno : -ETIMEDOUT;
	if (response[0] != SERVO_HEADER || response[1] != SERVO_HEADER ||
	    response[2] != servo_id || response[3] < 3)
		return -EBADMSG;
	checksum = response[2] + response[3] + response[4] +
		response[5] + response[6];
	if ((uint8_t)(~checksum) != response[7])
		return -EBADMSG;
	*position = response[5] | ((uint16_t)response[6] << 8);
	return 0;
}
