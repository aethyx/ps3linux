
KSRC_DIR=/lib/modules/$(shell uname -r)/build

obj-m += spuldrfs.o

all:
	make -C $(KSRC_DIR) M=$(PWD) modules

clean:
	make -C $(KSRC_DIR) M=$(PWD) clean
