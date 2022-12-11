KERNELDIR := ${TOPDIR}/linux-5.0
CURRENT_PATH := $(shell pwd)

obj-m := beep.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	${CROSS_COMPILE}gcc app_beep.c -o app_beep

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -rf app_beep
