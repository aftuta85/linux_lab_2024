CC = gcc
CFLAGS = -Wall

SRCS = swar.c
OBJS = $(SRCS:.c=.o)

main: $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o swar

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS) swar