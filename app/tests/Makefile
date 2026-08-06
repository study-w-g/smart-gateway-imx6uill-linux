# 用户态驱动测试程序 Makefile

CC ?= arm-linux-gnueabihf-gcc
CFLAGS ?= -Wall -Wextra -O2 -g -std=c11

PROGRAMS := test_ds18b20 test_sht30 test_gpio

.PHONY: all clean help

all: $(PROGRAMS)

test_ds18b20: test_ds18b20.c
	$(CC) $(CFLAGS) $< -o $@

test_sht30: test_sht30.c
	$(CC) $(CFLAGS) $< -o $@

test_gpio: test_gpio.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS)

help:
	@echo "构建驱动测试程序：make"
