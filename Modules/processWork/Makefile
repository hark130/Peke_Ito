MY_LKM = myProcessModule
MY_INC_DIR = $(src)/../../Include
ccflags-y := -I$(MY_INC_DIR)
obj-m += $(MY_LKM).o

# Ubuntu
KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)
# Fedora
# KERNEL_DIR = /usr/src/kernels/$(shell uname -r)

all:
	# Run everything from scratch
	$(MAKE) reset
	$(MAKE) lkm
	$(MAKE) test

clean:
	# Remove non-source controlled files
	rm -rf *.o *.ko *.mod.* *.symvers *.order *-

lkm:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

reset:
	# Reset all files back to the starting state
	$(MAKE) teardown
	$(MAKE) clean

teardown:
	# Unload the kernel module
	-sudo rmmod $(MY_LKM)

test:
	# We put a — in front of the rmmod command to tell make to ignore
	# an error in case the module isn’t loaded.
	-sudo rmmod $(MY_LKM)

	# Clear the kernel log without echo
	sudo dmesg -C

	# Insert the module
	sudo insmod $(MY_LKM).ko

	# Display the kernel log
	dmesg
