# 用户态网关服务 Makefile

PROGRAM := gateway-manager
SOURCES := gateway_manager.c mqtt_client.c

CC ?= arm-linux-gnueabihf-gcc
CFLAGS ?= -Wall -Wextra -O2 -g -std=c11
LIBS ?= -lmosquitto -lpthread -lm

.PHONY: all clean help

all: $(PROGRAM)

$(PROGRAM): $(SOURCES) mqtt_client.h
	$(CC) $(CFLAGS) -I. $(SOURCES) -o $@ $(LIBS)

clean:
	rm -f $(PROGRAM)

help:
	@echo "构建用户态网关服务："
	@echo "  make CC=arm-linux-gnueabihf-gcc"
	@echo "  make clean"
