obj-m := lkm.o

ARCH = arm
CROSS_COMPILE = arm-linux-gnueabihf-
KERN_DIR = /home/mukesh/Work/workspace/ldd/source/linux_bbb_4.14/
HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help
host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
copy:
	scp lkm.ko debian@192.168.7.2:/home/debian/drivers
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean
	make -C $(HOST_KERN_DIR) M=$(PWD) clean
