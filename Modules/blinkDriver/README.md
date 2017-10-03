## General Resources
* [blink firmware](https://github.com/todbot/blink1/blob/master/hardware/firmware/blink1.c)
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
* [Exploitation Report](http://kghosh.yoll.net/)

## Interrupt Handling
* [Chapter 7. Interrupts and Interrupt Handlers](https://notes.shichao.io/lkd/ch7/)
* [Linux Kernel Module Programming Guide 12.1. Interrupt Handlers](http://www.tldp.org/LDP/lkmpg/2.6/html/x1256.html)
* [Installing an Interrupt Handler](http://www.makelinux.net/ldd3/chp-10-sect-2)
* [Writing a Linux Kernel Module — Part 3: Buttons and LEDs](http://derekmolloy.ie/kernel-gpio-programming-buttons-and-leds/)
* [blink1 hid.c](https://github.com/todbot/blink1/blob/aedb03fcfb68461c570e4a76dc3b5d6a1d3a5e82/commandline/hidapi/linux/hid.c)

## Investigation
* https://github.com/todbot/blink1/blob/aedb03fcfb68461c570e4a76dc3b5d6a1d3a5e82/hardware/firmware/usbdrv/usbdrv.h
* https://github.com/todbot/blink1/blob/aedb03fcfb68461c570e4a76dc3b5d6a1d3a5e82/commandline/hidapi/linux/README.txt
* https://github.com/todbot/blink1/blob/aedb03fcfb68461c570e4a76dc3b5d6a1d3a5e82/hardware/firmware/usbconfig.h
* https://github.com/todbot/blink1/blob/aedb03fcfb68461c570e4a76dc3b5d6a1d3a5e82/hardware/firmware_mk2/main.c
* https://github.com/todbot/blink1/blob/aedb03fcfb68461c570e4a76dc3b5d6a1d3a5e82/hardware/firmware/usbdrv/usbportability.h
* https://github.com/todbot/blink1/blob/aedb03fcfb68461c570e4a76dc3b5d6a1d3a5e82/commandline/hidapi/libusb/hid.c
