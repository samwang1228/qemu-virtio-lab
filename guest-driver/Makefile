obj-m += virtio_custom.o

KDIR ?= ../../build/linux
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) ARCH=arm64 M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) ARCH=arm64 M=$(PWD) clean
