NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread

SRCS		= main.c parsing.c time.c heap.c heap_utils.c dongle.c simulation.c monitor.c
OBJS		= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c codexion.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
