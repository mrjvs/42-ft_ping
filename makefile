NAME=ft_ping
CC=gcc
FLAGS=-Werror -Wall -Wextra

INCLUDES=./src/includes

HEADERS=

SOURCES=\
	main.c

SOURCES_PREFIXED=$(addprefix ./src/, $(SOURCES))
HEADERS_PREFIXED=$(addprefix ./src/includes/, $(HEADERS))
OBJ=$(SOURCES_PREFIXED:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(FLAGS) -I $(INCLUDES) -o $@ $(OBJ)

%.o: %.c $(HEADERS_PREFIXED)
	$(CC) $(FLAGS) -I $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all
