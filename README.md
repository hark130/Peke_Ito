# Peke_Ito
Nagsasanay ako sa Linux system at umuunlad ng kernel.

## Instructions
### Hello.ko and myTestDeviceDriver.ko
    1.  make
    2.  sudo insmod 'driver'.ko
    3.  dmesg | tail -n 10
    4.  sudo mknod /dev/'driver' c <num> 0
    5.  sudo rmmod <driver>.ko
    6. rm /dev/'driver'
### blinkDeviceDriver.ko
    1.  make
    2.  sudo rmmod hid_generic
    3.  sudo rmmod usbhid
    4.  sudo rmmod hid_led
    5.  sudo rmmod hid
    6.  sudo insmod blinkDeviceDriver.ko

## To Do
    [X] Hello World lkm
    [X] Basic Character Driver
    [/] "Unk" USB Device Driver (blink(1), RGB LED)
        [ ] Get/create/allocate struct usb_interface for the device (...but how?)
        [ ] Convert between struct usb_interface and struct usb_device using interface_to_usbdev()? (see: Ch 13 Pg 332)
        [ ] Create an URB w/ usb_alloc_urb() (Init?)
        [ ] Fill in URB w/ usb_fill_int_urb() (Init?)
        [ ] Send data w/ usb_submit_urb() (Probe?  New function?)
        [ ] Handle URB Completion (see: Ch 13 - Completing Urbs: The Completion Callback Handler)
        [X] NOTE: Continue reading Ch 13 at "Writing a USB Driver"
        [ ] Dynamically read in the three strings (Make, Model, Serial #)
    [ ] "Unk" USB Device Driver (Nerf-style turret)
    [ ] [LKM to Intercept Syscalls](https://dl.packetstormsecurity.net/docs/hack/LKM_HACKING.html#II.1.)
    [ ] LKM to wiggle a cursor randomly
    [ ] Manage concurrent execution
        * include <linux/sched.h>
        * printk(KERN_INFO "The process is \"%s\" (pid %i)\n", current->comm, current->pid);
        * See: Linux Device Drivers Chapter 2
    [ ] Read [this](https://www.kernel.org/doc/html/v4.13/driver-api/usb/hotplug.html) to attempt to discover a reason why blinkDeviceDriver is failing to 'hotplug'


