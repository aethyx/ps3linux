
CC=gcc
CFLAGS=-O2 -Wall
LDFLAGS=
SRC=des_cbc.c
OBJ=$(SRC:.c=.o)
LIB=-lcrypto -lssl

TARGET=des_cbc

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
