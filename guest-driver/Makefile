obj-m += virtio_custom.o

KDIR ?= /repo/build/linux
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
