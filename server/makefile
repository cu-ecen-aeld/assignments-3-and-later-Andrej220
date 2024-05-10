.PHONY: all, clean

CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Wextra -g 
LDFLAGS ?= -pthread -lrt
SRCS =  aesdsocket_linkedlists.c
>>>>>>> 794f6ac488da03841313127af43b925172e9a5ae
OBJS = $(SRCS:.c=.o)
TARGET = aesdsocket

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)
