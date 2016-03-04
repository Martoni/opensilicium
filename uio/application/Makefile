
BSP_APF6_BROUTPUT := /usr/local/projects/armadeus/apf6/buildroot/output/

CC = $(BSP_APF6_BROUTPUT)host/usr/bin/arm-linux-gnueabihf-gcc
LD = $(BSP_APF6_BROUTPUT)host/usr/bin/arm-linux-gnueabihf-ld

default: blinkled

blinkled : blinkled.o ../uio_helper/uio_helper.o

clean:
	-rm ../uio_helper/uio_helper.o
	-rm blinkled.o
