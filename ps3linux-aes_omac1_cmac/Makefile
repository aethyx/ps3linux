
CC=gcc
CFLAGS=-O2 -Wall
LDFLAGS=
SRC=aes_omac1_cmac.c
OBJ=$(SRC:.c=.o)
LIB=-lcrypto -lssl

TARGET=aes_omac1_cmac

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
