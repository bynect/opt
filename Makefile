CC=gcc
CFLAGS=-g -O2 -Wall

SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
BIN=test

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $^

.PHONY: clean
clean:
	rm -f $(OBJ) $(BIN)
