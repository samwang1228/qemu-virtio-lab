obj-m += virtio_custom.o
virtio_custom-y := virtio_custom_main.o virtio_custom_dma.o virtio_custom_sysfs.o

ccflags-y += -I$(src)/../include -I$(src)

KDIR ?= /repo/build/linux
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
