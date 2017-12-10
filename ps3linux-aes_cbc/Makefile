
CC=gcc
CFLAGS=-O2 -Wall
LDFLAGS=
SRC=aes_cbc.c
OBJ=$(SRC:.c=.o)
LIB=-lcrypto -lssl

TARGET=aes_cbc

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
