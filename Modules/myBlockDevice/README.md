# LKM Block Device Driver

## Description

- Practice with a Linux Kernel Module block device driver to a virtual block device

## Steps

1. Create a virtual block device
2. Create a Linux Kernel Module for that block device
3. Write a program to interact with that device
4. Write a program to verify the results

## References

- Virtual Block Devices
	- [How to create virtual block device (loop device/filesystem) in Linux](https://www.thegeekdiary.com/how-to-create-virtual-block-device-loop-device-filesystem-in-linux/)
	- [Use A File As A Linux Block Device](https://www.jamescoyle.net/how-to/2096-use-a-file-as-a-linux-block-device)
	- [How to Create Virtual Block Devices in Ubuntu](https://askubuntu.com/questions/546921/how-to-create-virtual-block-devices)
	- [How to create virtual block devices from file?](https://superuser.com/questions/1033493/how-to-create-virtual-block-devices-from-file)
- Block Device Drivers
	- [Linux Device Drivers 3 - Ch 16 Block Drivers](https://static.lwn.net/images/pdf/LDD3/ch16.pdf)
	- [A Simple Block Driver for Linux Kernel 2.6.31](https://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/)
	- [Virtual Linux block device driver for simulating and performing I/O](https://github.com/rgolubtsov/virtblkiosim)
	- [The Linux Kernel - Block Device Drivers](https://linux-kernel-labs.github.io/master/labs/block_device_drivers.html)
	- [Linux Journal - An introduction to block device drivers](https://www.linuxjournal.com/article/2890)
	- [Block Drivers](https://bootlin.com/doc/legacy/block-drivers/block_drivers.pdf)

## Notes

### 1. Create a virtual block device

	**NOTE:** Drawn heavily from [How to create virtual block device (loop device/filesystem) in Linux](https://www.thegeekdiary.com/how-to-create-virtual-block-device-loop-device-filesystem-in-linux/)

#### A. Create a file

	1. First step is to create a file of desired size
		- The dd Unix utility program reads octet streams from a source to a destination, possibly performing data conversions in the process. Destroying existing data on a file system partition (low-level formatting):
		- ```dd if=/dev/zero of=virtBlockFile0.img bs=10M count=10```
	2. Verify the size of the file you have just created
		- Summarizes file space usage in a human-readable format
		- ```du -sh virtBlockFile0.img```

#### B. Create the loop device

	1. Create a loop device with the file. Use the command “losetup” to create a loop device “loop0”
		- f – find the first unused loop device. If a file argument is present, use this device. Otherwise, print its name.
		- P – force kernel to scan partition table on newly created loop device.
		- ```losetup -fP virtBlockFile0.img``` grabs the first available loop device name
		- ```mknod /dev/virtBlockDev0 b 7 1337``` manually creates a device file so you can...
		- ```losetup -P /dev/virtBlockDev0 virtBlockFile0.img``` to use your manually created device file instead of /dev/loop?
	2. Print the loop device generated using the above command use “losetup -a”
		- ```losetup -a```

#### C. Removing the loop device

	1. Delete the loopback device created using the “losetup -d” command
		- ```losetup -d /dev/loop0```
		- ```losetup -d /dev/virtBlockDev0```

#### D. Deleting the file

	1. Remove the file used to create the loop device
		- ```rm virtBlockFile0.img```
