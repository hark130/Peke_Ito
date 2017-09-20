# Peke_Ito
Nagsasanay ako sa Linux system at umuunlad ng kernel.

## Instructions
    1.  make
    2.  sudo insmod 'driver'.ko
    3.  dmesg | tail -n 10
    4.  sudo mknod /dev/'driver' c <num> 0
    5.  sudo rmmod <driver>.ko

## To Do
    [X] Hello World lkm
    [X] Basic Character Driver
    [/] "Unk" USB Device Driver (blink(1), RGB LED)
    [ ] "Unk" USB Device Driver (Nerf-style turret)

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