# DS18B20 external kernel module Makefile

obj-m += ds18b20.o

KERNEL_DIR ?= /path/to/linux-kernel
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

.PHONY: all clean help

all:
	@if [ "$(KERNEL_DIR)" = "/path/to/linux-kernel" ]; then \
		echo "Please set KERNEL_DIR to the target Linux kernel source directory."; \
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
		echo "KERNEL_DIR is not available; removing common local build outputs."; \
		rm -f *.o *.ko *.mod *.mod.c Module.symvers modules.order; \
	fi

help:
	@echo "Build DS18B20 external module:"
	@echo "  make KERNEL_DIR=/path/to/linux-kernel CROSS_COMPILE=arm-linux-gnueabihf-"
	@echo "  make clean KERNEL_DIR=/path/to/linux-kernel"
