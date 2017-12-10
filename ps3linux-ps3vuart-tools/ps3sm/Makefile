
LIBCMD_DIR=../cmd
LIBCMD=$(LIBCMD_DIR)/libcmd.a

LIBUTIL_DIR=../util
LIBUTIL=$(LIBUTIL_DIR)/libutil.a

CC=gcc
CFLAGS=-Wall -g -O2
LDFLAGS=-L$(LIBCMD_DIR) -L$(LIBUTIL_DIR)
SRC=ps3sm.c dev.c $(wildcard cmd/*.c)
OBJ=$(SRC:.c=.o)
INC=-I$(LIBCMD_DIR) -I$(LIBUTIL_DIR)
LIB=-lcmd -lutil
TARGET=ps3sm

all: $(TARGET)

$(TARGET): $(LIBCMD) $(LIBUTIL) $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

.PHONY: $(LIBCMD)
$(LIBCMD):
	$(MAKE) -C $(LIBCMD_DIR)

.PHONY: $(LIBUTIL)
$(LIBUTIL):
	$(MAKE) -C $(LIBUTIL_DIR)

.PHONY: clean
clean:
	$(MAKE) -C $(LIBCMD_DIR) clean
	$(MAKE) -C $(LIBUTIL_DIR) clean
	rm -f $(OBJ) $(TARGET)
