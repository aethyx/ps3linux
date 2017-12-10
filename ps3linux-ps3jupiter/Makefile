
KSRC_DIR=/lib/modules/$(shell uname -r)/build

obj-m += ps3_jupiter.o
obj-m += ps3_jupiter_sta.o
obj-m += ps3_jupiter_ap.o

all:
	make -C $(KSRC_DIR) M=$(PWD) modules

clean:
	make -C $(KSRC_DIR) M=$(PWD) clean
