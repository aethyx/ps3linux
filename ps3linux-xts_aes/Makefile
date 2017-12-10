
CC=gcc
CFLAGS=-O2 -Wall
LDFLAGS=
SRC=xts_aes.c
OBJ=$(SRC:.c=.o)
LIB=-lcrypto -lssl

TARGET=xts_aes

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
