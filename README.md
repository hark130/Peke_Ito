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
### Key Logger v1.0
#### BRIEF

- Creates /dev/notakeylogger to communicate with a user
- Registers a module with the keyboard driver's notification list
- Translates those scan codes into a buffer backing /dev/notakeylogger
- Key strokes may be read from /dev/notakeylogger

#### USAGE

1. ```cd Modules/myKeyLogger; make clean; make lkm```
2. ```sudo insmod myKeyLoggingModule.ko```
3. Type things
4. ```sudo cat /dev/notakeylogger```
5. Type more things
6. ```sudo cat /dev/notakeylogger```
7. ```sudo rmmod myKeyLoggingModule```

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
        [ ] Dynamically read in the three strings (Make, Model, Serial #)
    [ ] "Unk" USB Device Driver (Nerf-style turret)
    [ ] [LKM to Intercept Syscalls](https://dl.packetstormsecurity.net/docs/hack/LKM_HACKING.html#II.1.)
    [ ] Manage concurrent execution
        * include <linux/sched.h>
        * printk(KERN_INFO "The process is \"%s\" (pid %i)\n", current->comm, current->pid);
        * See: Linux Device Drivers Chapter 2
    [ ] Read [this](https://www.kernel.org/doc/html/v4.13/driver-api/usb/hotplug.html) to attempt to discover a reason why blinkDeviceDriver is failing to 'hotplug'

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
* [Kernel Data Types](http://ww2.cs.fsu.edu/~stanovic/teaching/ldd_summer_2014/notes/lecture_data_types.ppt)
* [USB Descriptor Tables](http://www.beyondlogic.org/usbnutshell/usb5.shtml)
* [USB Endpoint Descriptor Details](https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__endpoint__descriptor.html)
* [USB Interrupt Transfers in a Nutshell](http://www.beyondlogic.org/usbnutshell/usb4.shtml#Interrupt)
* [USB Made Simple](http://www.usbmadesimple.co.uk/ums_4.htm)
* [USB Complete 3rd Edition](http://s.eeweb.com/members/mark_harrington/answers/1333179451-USB_Complete_3rdEdition.pdf)
* [USB HID Report Descriptors](http://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/)
* [Reverse Engineering SET_REPORT Packets](https://hackaday.io/project/5301/logs)
* [Universal Serial Bus Specification v2.0](sdphca.ucsd.edu/lab_equip_manuals/usb_20.pdf)
* Most drivers do not use 'usb_fill_control_urb', as it is much simpler to use the synchronous API calls as described in [Chapter 13 Section 5](http://www.makelinux.net/ldd3/chp-13-sect-5#chp-13-sect-5)
* [Read and Write USB Device](https://sysplay.in/blog/tag/usb-data-transfer/)
* [struct urb Man Page](https://www.systutorials.com/docs/linux/man/9-struct_urb/)
* [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/html/lkmpg.html)
* [LKM Online Reading List](http://www.dit.upm.es//~jmseyas/linux/kernel/hackers-docs.html) - Referenced at the end of the [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/html/lkmpg.html)
* [(nearly) Complete Linux Loadable Kernel Modules](https://dl.packetstormsecurity.net/docs/hack/LKM_HACKING.html)
* [Interrupt vs Polling Chararacter Drivers](http://en.tldp.org/LDP/khg/HyperNews/get/devices/char.html)
* [Common Device Driver Functionality](http://en.tldp.org/LDP/khg/HyperNews/get/devices/reference.html)
* [Phrack](http://www.phrack.org/) - an ezine written by and for hackers 

## Ideas
* Phantom Keystroker from ThinkGeek
