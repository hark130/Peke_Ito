MY_LKM = myBlockDeviceDriver
obj-m += $(MY_LKM).o

# Ubuntu
# KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)
# Fedora
KERNEL_DIR = /usr/src/kernels/$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *-

test:
	# lsmod to grep the loaded kernel modules for our LKM
	# lsmod | grep myBlockDeviceDriver

	# We put a — in front of the rmmod command to tell make to ignore
	# an error in case the module isn’t loaded.
	-sudo rmmod $(MY_LKM)

	# Clear the kernel log without echo
	sudo dmesg -C

	# Insert the module
	sudo insmod $(MY_LKM).ko
	
	# Display the kernel log
	dmesg
