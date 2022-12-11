KERNELDIR := ${TOPDIR}/linux-5.0
CURRENT_PATH := $(shell pwd)

obj-m := timer.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	${CROSS_COMPILE}gcc app_timer.c -o app_timer

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -rf app_timer
