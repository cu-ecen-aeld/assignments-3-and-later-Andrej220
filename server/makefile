.PHONY: all, clean

CC := $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -g 
LDFLAGS = -pthread -lrt
SRCS = aesdsocket_linkedlists.c
OBJS = $(SRCS:.c=.o)
TARGET = aesdsocket

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)
