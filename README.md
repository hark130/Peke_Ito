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
