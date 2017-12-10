
AR=ar
CC=gcc
CFLAGS=-Wall -g -O2
SRC=util.c
OBJ=$(SRC:.c=.o)
LIB=
TARGET=libutil.a

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) cr $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
