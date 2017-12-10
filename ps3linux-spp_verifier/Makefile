
CC=gcc
CFLAGS+=-O2 -Wall -g
LDFLAGS+=
SRC=spp_verifier.c
OBJ=$(SRC:.c=.o)
LIB+=
TARGET=spp_verifier

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
