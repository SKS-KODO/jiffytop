CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g

INCLUDES = -Iinclude

SRC = src/main.c src/proc_read.c src/cpu_monitor.c src/memory_monitor.c

TARGET = proc_master

all:
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
