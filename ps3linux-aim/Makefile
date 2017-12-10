
CC=gcc
CFLAGS+=-O2 -Wall -g
LDFLAGS+=
SRC=aim.c
OBJ=$(SRC:.c=.o)
LIB+=
TARGET=aim

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
