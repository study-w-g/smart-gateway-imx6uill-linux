# SHT30 外部内核模块 Makefile

obj-m += sht30.o

KERNEL_DIR ?= /path/to/linux-kernel
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

.PHONY: all clean help

all:
	@if [ "$(KERNEL_DIR)" = "/path/to/linux-kernel" ]; then \
		echo "请将 KERNEL_DIR 设置为目标 Linux 内核源码目录。"; \
		exit 1; \
	fi
	$(MAKE) -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(CURDIR) \
		modules

clean:
	@if [ -d "$(KERNEL_DIR)" ]; then \
		$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean; \
	else \
		echo "KERNEL_DIR 不可用，正在删除常见的本地构建产物。"; \
		rm -f *.o *.ko *.mod *.mod.c Module.symvers modules.order; \
	fi

help:
	@echo "构建 SHT30 外部内核模块："
	@echo "  make KERNEL_DIR=/path/to/linux-kernel CROSS_COMPILE=arm-linux-gnueabihf-"
	@echo "  make clean KERNEL_DIR=/path/to/linux-kernel"
