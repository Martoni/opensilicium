
# Part executed when called from kernel build system:
ifneq ($(KERNELRELEASE),)

obj-m += uio_led.o

else

PWD := $(shell pwd)

BSP_APF6_BROUTPUT := /usr/local/projects/armadeus/apf6/buildroot/output/

CC = $(BSP_APF6_BROUTPUT)host/usr/bin/arm-linux-gnueabihf-gcc
LD = $(BSP_APF6_BROUTPUT)host/usr/bin/arm-linux-gnueabihf-ld
OBJDUMP = $(BSP_APF6_BROUTPUT)host/usr/bin/arm-linux-gnueabihf-objdump
KDIR := $(BSP_APF6_BROUTPUT)build/linux-3.19.8/

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux- modules CC=$(CC) LD=$(LD) OBJDUMP=$(OBJDUMP)

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -f modules.order
	rm -rf .tmp_versions/
	rm -f Module.markers

endif
