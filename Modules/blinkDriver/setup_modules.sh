#!/bin/bash
echo Did you sudo?  Most error messages are silenced.
rmmod blinkDeviceDriver 2> /dev/null
rmmod hid_led 2> /dev/null
rmmod usbhid 2> /dev/null
rmmod usbhid 2> /dev/null
rmmod hid_generic 2> /dev/null
rmmod hid 2> /dev/null
insmod blinkDeviceDriver.ko
