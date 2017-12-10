
CC=gcc
CFLAGS=-O2 -Wall -g
LDFLAGS=
LIB=-lpthread -lusb-1.0

all: ps3_jupiter_ap

ps3_jupiter_ap: util.o ps3_jupiter.o ps3_jupiter_ap.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f util.o ps3_jupiter.o ps3_jupiter_ap.o ps3_jupiter_ap
