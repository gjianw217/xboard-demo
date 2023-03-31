KERN_DIR = /home/gjw/xboard/kernel/udoo-neo-imx6sx-kernel
CROSS_COMPILE =/home/gjw/xboard/tool/toolchain/neo/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
all:
	make -C $(KERN_DIR)  M=$(shell pwd) modules
	make -C $(shell pwd)/oled_app 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	make -C $(shell pwd)/oled_app  clean

oled-y := oled_drv.o 
obj-m  += oled.o

install:
	sudo insmod oled.ko

uninstall:
	sudo rmmod oled
