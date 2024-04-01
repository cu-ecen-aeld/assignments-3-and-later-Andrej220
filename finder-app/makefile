.PHONY: all, clean

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra
SRCS = writer.c
OBJS = $(SRCS:.c=.o)
TARGET = writer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(OBJS)
