MY_LKM = myBlockDeviceDriver
obj-m += $(MY_LKM).o
BLOCK_FILE = virtBlockFile0
BLOCK_FILENAME = $(BLOCK_FILE).img
BLOCK_SIZE = 10M
BLOCK_COUNT = 10
BLOCK_DEV = virtBlockDev0
DEV_MAJ_NUM = 7
DEV_MIN_NUM = 1337

# Ubuntu
# KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)
# Fedora
KERNEL_DIR = /usr/src/kernels/$(shell uname -r)

all:
	# Run everything from scratch
	$(MAKE) reset
	$(MAKE) setup
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

setup:
	# 1. Create the file
	# 1.1. Zeroize the file
	dd if=/dev/zero of=$(BLOCK_FILENAME) bs=$(BLOCK_SIZE) count=$(BLOCK_COUNT)
	# 1.2. Print the size
	du -sh $(BLOCK_FILENAME)

	# 2. Create the loop device
	# 2.1. Create a special device file
	sudo mknod /dev/$(BLOCK_DEV) b $(DEV_MAJ_NUM) $(DEV_MIN_NUM)
	# 2.2. Make that special device file a loop block device
	sudo losetup -P /dev/$(BLOCK_DEV) $(BLOCK_FILENAME)

teardown:
	# Unload the kernel module
	-sudo rmmod $(MY_LKM)
	# Delete loop device
	-sudo losetup -d /dev/$(BLOCK_DEV)
	# Delete the backing file
	-rm -f $(BLOCK_FILENAME)
	# Delete the special device file
	-sudo rm -f /dev/$(BLOCK_DEV)	

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
