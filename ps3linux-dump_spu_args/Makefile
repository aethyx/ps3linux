
CC=gcc
CFLAGS+=-O2 -Wall -g
LDFLAGS+=
SRC=dump_spu_args.c
OBJ=$(SRC:.c=.o)
LIB+=
TARGET=dump_spu_args

SPU_GCC=spu-gcc
SPU_OBJCOPY=spu-objcopy
SPU_SRC=spu.S
SPU_OBJ=$(SPU_SRC:.S=.o)
SPU_TARGET=spu.bin

all: $(TARGET) $(SPU_TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

$(SPU_TARGET): $(SPU_OBJ)
	$(SPU_OBJCOPY) -O binary $< $@

$(SPU_OBJ): $(SPU_SRC)
	$(SPU_GCC) -c $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
	rm -f $(SPU_OBJ) $(SPU_TARGET)
