obj-m := main.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
KERN_DIR=/home/amol/Projects/BBB/linux/
HOST_KERN_DIR=/lib/modules/5.11.0-38-generic/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help

host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules

host-clean:
	make -C $(HOST_KERN_DIR) M=$(PWD) clean