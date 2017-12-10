
KSRC_DIR=/lib/modules/$(shell uname -r)/build

ps3lv1call-objs := ps3lv1call_misc.o generic_lv1call.o
obj-m += ps3lv1call.o

all:
	make -C $(KSRC_DIR) M=$(PWD) modules

clean:
	make -C $(KSRC_DIR) M=$(PWD) clean
