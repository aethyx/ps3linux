
CC=gcc
CFLAGS=-Wall -g -O2
LDFLAGS=
SRC=ps3sed.c dev.c $(wildcard cmd/*.c)
OBJ=$(SRC:.c=.o)
INC=
LIB=
TARGET=ps3sed

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
