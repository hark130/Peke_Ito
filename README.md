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
        [ ] NOTE: Continue reading Ch 13 at "Writing a USB Driver"
    [ ] "Unk" USB Device Driver (Nerf-style turret)
    [ ] Manage concurrent execution
        * include <linux/sched.h>
        * printk(KERN_INFO "The process is \"%s\" (pid %i)\n", current->comm, current->pid);
        * See: Linux Device Drivers Chapter 2

## Resources
* http://opensourceforu.com/2011/10/usb-drivers-in-linux-1/
* http://matthias.vallentin.net/blog/2007/04/writing-a-linux-kernel-driver-for-an-unknown-usb-device/
* https://kernel.readthedocs.io/en/sphinx-samples/writing_usb_driver.html
* http://www.linuxjournal.com/article/7353
* https://www.safaribooksonline.com/library/view/linux-device-drivers/0596005903/ch13.html
* http://www.tldp.org/HOWTO/html_single/Module-HOWTO/
* http://derekmolloy.ie/kernel-gpio-programming-buttons-and-leds/
* http://www.beyondlogic.org/usbnutshell/usb6.shtml
* http://www.usbmadesimple.co.uk/ums_4.htm
* https://msdn.microsoft.com/en-us/library/windows/hardware/ff539261(v=vs.85).aspx
* https://stackoverflow.com/questions/9505105/usb-getting-data-from-a-device
* https://github.com/todbot/blink1
* [Linux Device Drivers 3rd Edition](http://www.free-electrons.com/doc/books/ldd3.pdf)
* [Write/Read USB Device](http://opensourceforu.com/2011/12/data-transfers-to-from-usb-devices/)
* [USB Device Driver slideshow by Michael Mitchell](ww2.cs.fsu.edu/~stanovic/teaching/ldd_summer_2014/notes/usb/usb_drivers.ppt)
* Peke_Ito/Kernels/linux-4.4.79/Documentation/usb/URB.txt
