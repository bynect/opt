CC=gcc
CFLAGS=-O3 -Wall -Werror

SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
BIN=test
LIB=libopt.a

all: $(LIB)

$(BIN): $(OBJ)
	$(CC) -o $@ $^

$(LIB): opt.o
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $^

.PHONY: clean
clean:
	rm -f $(OBJ) $(BIN) $(LIB)
