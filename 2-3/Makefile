CC = gcc
CFLAGS = -Wall

SRCS = find_nth_bit.c
OBJS = $(SRCS:.c=.o)

main: $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o find_nth_bit

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) find_nth_bit