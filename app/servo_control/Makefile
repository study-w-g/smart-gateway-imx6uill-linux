# LX-16A 串口舵机控制程序 Makefile

CC ?= arm-linux-gnueabihf-gcc
CFLAGS ?= -Wall -Wextra -O2 -g -std=c11 -D_DEFAULT_SOURCE

.PHONY: all clean help

all: libservo_control.a

libservo_control.a: servo_control.o
	ar rcs $@ $^

servo_control.o: servo_control.c servo_control.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.a

help:
	@echo "构建 LX-16A 舵机控制库：make"
