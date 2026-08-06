# GPIO 按键/LED 外部内核模块 Makefile

obj-m += gpio_event.o

ccflags-y += -I$(src)

KERNEL_DIR ?= /path/to/linux-kernel
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

.PHONY: all clean help

all:
	@if [ "$(KERNEL_DIR)" = "/path/to/linux-kernel" ]; then \
		echo "请将 KERNEL_DIR 设置为目标 Linux 内核源码目录。"; \
		exit 1; \
	fi
	$(MAKE) -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(CURDIR) modules

clean:
	@if [ -d "$(KERNEL_DIR)" ]; then \
		$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean; \
	else \
		rm -f *.o *.ko *.mod *.mod.c Module.symvers modules.order; \
	fi

help:
	@echo "构建 GPIO 按键/LED 驱动："
	@echo "  make KERNEL_DIR=/path/to/linux-kernel CROSS_COMPILE=arm-linux-gnueabihf-"
