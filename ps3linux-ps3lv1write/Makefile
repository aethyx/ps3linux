
CC=gcc
CFLAGS=-O2 -Wall -g
LDFLAGS=

all: ps3lv1write

ps3lv1write: ps3lv1write.o dev.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f dev.o
	rm -f ps3lv1write.o ps3lv1write
