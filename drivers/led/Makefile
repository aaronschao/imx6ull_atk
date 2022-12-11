KERNELDIR := ${TOPDIR}/linux-5.0
CURRENT_PATH := $(shell pwd)

obj-m := led.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	${CROSS_COMPILE}gcc app_led.c -o app_led

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -rf app_led
