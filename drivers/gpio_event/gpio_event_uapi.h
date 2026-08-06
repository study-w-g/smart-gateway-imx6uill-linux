#ifndef SMART_GATEWAY_GPIO_EVENT_UAPI_H
#define SMART_GATEWAY_GPIO_EVENT_UAPI_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
typedef uint64_t __u64;
typedef uint32_t __u32;
#endif

/*
 * 这个头文件同时被内核驱动和用户态程序使用。
 * 这样 ioctl 编号和事件结构体不会在两边各写一份而产生不一致。
 */
struct gpio_event_record {
	__u64 timestamp_ns;
	__u32 value;
	__u32 sequence;
};

#define GPIO_EVENT_IOC_MAGIC   'G'
#define GPIO_EVENT_IOC_SET_LED _IOW(GPIO_EVENT_IOC_MAGIC, 0x01, __u32)
#define GPIO_EVENT_IOC_GET_LED _IOR(GPIO_EVENT_IOC_MAGIC, 0x02, __u32)

#endif
