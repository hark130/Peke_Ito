obj-m += Hello.o

KDIR = /usr/src/linux-headers-4.8.0-36-generic


all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.* *.syvers *.order
