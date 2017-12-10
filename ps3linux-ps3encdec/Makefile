
KSRC_DIR=/lib/modules/$(shell uname -r)/build

obj-m += ps3encdec.o

all:
	make -C $(KSRC_DIR) M=$(PWD) modules

clean:
	make -C $(KSRC_DIR) M=$(PWD) clean
