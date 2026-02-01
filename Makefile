NAME = tshoo_line.a 
CC = cc -Wall -Wextra -Werror -Iincludes
CCDEBUG = $(CC) -g3 -O0

SRC_DIR = src
SRC_FILES = history_handling.c \
			raw_mode_handling.c \
			completion.c \
			tshoo_line.c

SRC = $(addprefix $(SRC_DIR)/, $(SRC_FILES))

OBJ_DIR = obj
OBJ_FILES = $(SRC_FILES:.c=.o)

OBJ = $(addprefix $(OBJ_DIR)/, $(OBJ_FILES))

all: $(NAME)

$(NAME): $(OBJ)
	ar rcs $(NAME) $(OBJ)

$(OBJ): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@

debug: fclean
	$(MAKE) CC="$(CCDEBUG)" all

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all debug clean fclean re
