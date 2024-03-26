CC = gcc
CFLAGS = -Wall -g

SRCS = main.c timsort.c timsort_orig.c
OBJS = $(SRCS:.c=.o)

main: $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o main

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) main