NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread

SRCS		= coders/main.c coders/parsing.c coders/time.c \
			  coders/heap.c coders/heap_utils.c coders/dongle.c \
			  coders/dongle_take.c coders/dongle_pair.c \
			  coders/dongle_wait.c coders/log.c coders/coder.c \
			  coders/simulation.c coders/monitor.c

OBJS		= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c coders/codexion.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
