
CC=gcc
CFLAGS=-O2 -Wall
LDFLAGS=
SRC=ps3physmem_test.c
OBJ=$(SRC:.c=.o)
LIB=

TARGET=ps3physmem_test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
