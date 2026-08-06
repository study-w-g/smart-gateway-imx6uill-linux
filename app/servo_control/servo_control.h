#ifndef SMART_GATEWAY_SERVO_CONTROL_H
#define SMART_GATEWAY_SERVO_CONTROL_H

#include <stdint.h>

/* LX-16A 常用控制接口，实际串口节点和波特率由开发板配置决定。 */
int servo_open(const char *device, int baudrate);
void servo_close(int fd);
int servo_move(int fd, uint8_t servo_id, uint16_t position,
	       uint16_t time_ms);
int servo_read_position(int fd, uint8_t servo_id, uint16_t *position);

#endif
