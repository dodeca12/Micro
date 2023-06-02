.PHONY: all clean

OS := $(shell uname)

ifeq ($(OS), Darwin)
	CC := clang
else
	CC := gcc
endif

SRC := micro.c input.c output.c bufferOperations.c search.c fileIO.c microOperations.c rowOperations.c syntaxHighlighting.c terminalManipulation.c
OBJ := $(SRC:.c=.o)

all: micro

micro: $(OBJ)
	$(CC) -o micro $(OBJ) -Wall -Werror -Wextra -pedantic -std=c99
	rm -f $(OBJ)

%.o: %.c
	$(CC) -c $<

clean:
	rm -f micro $(OBJ)
