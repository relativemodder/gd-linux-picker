CC      = winegcc
CFLAGS  = -O2 -Wall -static
LDFLAGS =

TARGET  = starterproxy
SRCS    = main.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
