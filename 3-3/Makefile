CC = gcc
CFLAGS = -Wall

SRCS = ilog2.c
OBJS = $(SRCS:.c=.o)

main: $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o ilog2

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS) ilog2