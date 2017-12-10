
AR=ar
CC=gcc
CFLAGS=-Wall -g -O2
SRC=cmd.c cmd_list.c cmd_help.c
OBJ=$(SRC:.c=.o)
LIB=
TARGET=libcmd.a

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) cr $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
