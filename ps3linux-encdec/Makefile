
CC=gcc
CFLAGS=-O2 -Wall -fno-strict-aliasing
LDFLAGS=-lcrypto -lssl
SRC=encdec.c
OBJ=$(SRC:.c=.o)

TARGET=encdec

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
