
AR=ar
CC=gcc
CFLAGS=-Wall -O2 -g
LDFLAGS=
SRC=spu.c spufs.c spuisofs.c spuldrfs.c
OBJ=$(SRC:.c=.o)
TARGET=libspufs.a

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) cr $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
