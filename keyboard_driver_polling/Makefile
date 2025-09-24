obj-m += keyboard_driver.o

all: 
	make -C /usr/src/linux-$(shell uname -r) SUBDIRS=$(shell pwd) modules

clean:
	make -C /usr/src/linux-$(shell uname -r) SUBDIRS=$(shell pwd) clean

