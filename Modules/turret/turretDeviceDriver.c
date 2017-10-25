
#include <linux/module.h>       // ALWAYS NEED
#include <linux/kernel.h>       // ALWAYS NEED
#include <linux/slab.h>         // kmalloc
#include <linux/usb.h>          // Always needed for USB descriptors

//////////////////////////////////////////////////////////////////////
/////////////////////////////// MACROS ///////////////////////////////
//////////////////////////////////////////////////////////////////////

#define DEVICE_NAME "USB Turret"
// Get these values from 'lsusb -v'
#define VENDOR_ID   0x0000
#define PRODUCT_ID  0x0000

#define ML_STOP         0x00
#define ML_UP           0x01
#define ML_DOWN         0x02
#define ML_LEFT         0x04
#define ML_RIGHT        0x08
#define ML_UP_LEFT      (ML_UP | ML_LEFT)
#define ML_DOWN_LEFT    (ML_DOWN | ML_LEFT)
#define ML_UP_RIGHT     (ML_UP | ML_RIGHT)
#define ML_DOWN_RIGHT   (ML_DOWN | ML_RIGHT)
#define ML_FIRE         0x10

#define ML_MAX_UP       0x80        /* 80 00 00 00 00 00 00 00 */
#define ML_MAX_DOWN     0x40        /* 40 00 00 00 00 00 00 00 */
#define ML_MAX_LEFT     0x04        /* 00 04 00 00 00 00 00 00 */
#define ML_MAX_RIGHT    0x08        /* 00 08 00 00 00 00 00 00 */

//////////////////////////////////////////////////////////////////////
/////////////////////////// GLOBAL STRUCTS ///////////////////////////
//////////////////////////////////////////////////////////////////////

// Describes type of USB devices this driver supports
// Adds VendorID and Product ID to the usb_device_id struct
static struct usb_device_id turret_table[] =
{
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {} // Terminating entry
};
MODULE_DEVICE_TABLE(usb, turret_table);

// USB Driver struct
static struct usb_driver turret_driver =
{
    .name = DEVICE_NAME,
    .id_table = turret_table,
    .probe = turret_probe,
    .disconnect = turret_disconnect,
};

// Device File Operations
struct file_operations turretOps =
{
    .owner = THIS_MODULE,       // Prevent unloading of this module when operations are in use
    .open = turret_open,        // Points to the method to call when opening the device
    .release = turret_close,    // Points to the method to call when closing the device
    .write = turret_write,      // Points to the method to call when writing to the device
    // Will we need turret_read?  Is stubbing it out a good idea?
    .read = turret_read,        // Points to the method to call when reading from the device
};

// USB Device struct
static struct usb_device* turretDevice;  // Initialized by interface_to_usbdev(interface) in turret_probe()

struct usb_host_interface *iface_desc;

struct usb_endpoint_descriptor *endpoint;

//////////////////////////////////////////////////////////////////////
////////////////////////// GLOBAL VARIABLES //////////////////////////
//////////////////////////////////////////////////////////////////////
int major_number;               // Will store our major number - extracted from dev_t using macro - mknod /director/file c major minor
int retVal;                     // Will be used to hold return values of functions
int i;                          // Iterating variable


//////////////////////////////////////////////////////////////////////
//////////////////////// FUNCTION DEFINITIONS ////////////////////////
//////////////////////////////////////////////////////////////////////


////////////////////// USB DRIVER: TURRET_PROBE //////////////////////
static int turret_probe(struct usb_interface* interface, const struct usb_device_id* id)
{
    retVal = 0;

    blinkDevice = interface_to_usbdev(interface);
    
    /* FIND AN ENDPOINT */
    printk(KERN_INFO "%s: This interface has %u different settings.\n", DEVICE_NAME, interface->num_altsetting);
    retVal |= interface->cur_altsetting->desc.bNumEndpoints;  // Number of endpoints for this interface
    printk(KERN_INFO "%s: This interface has %d endpoints.\n", DEVICE_NAME, retVal);
    for (i = 0; i < interface->cur_altsetting->desc.bNumEndpoints; i++)
    {
        retVal = 0;
        retVal |= interface->cur_altsetting->endpoint[i].desc.bDescriptorType;
        printk(KERN_INFO "%s: Endpoint #%d has bDescriptorType: %d.\n", DEVICE_NAME, i, retVal);
        retVal = 0;
        retVal |= interface->cur_altsetting->endpoint[i].desc.bEndpointAddress;
        printk(KERN_INFO "%s: Endpoint #%d has bEndpointAddress: %d.\n", DEVICE_NAME, i, retVal);
        retVal = 0;
        retVal |= interface->cur_altsetting->endpoint[i].desc.bmAttributes;
        printk(KERN_INFO "%s: Endpoint #%d has bmAttributes: %d.\n", DEVICE_NAME, i, retVal);
        
/*
        endpoint = &iface_desc->endpoint[i].desc;

        if (((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
                && ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
                    USB_ENDPOINT_XFER_INT))
            dev->int_in_endpoint = endpoint;
*/
    }
}
//////////// USB DRIVER: IMPLEMENT TURRET_DISCONNECT ////////////
//////////// FILE OPERATIONS: IMPLEMENT TURRET_OPEN ////////////
//////////// FILE OPERATIONS: IMPLEMENT TURRET_CLOSE ////////////
//////////// FILE OPERATIONS: IMPLEMENT TURRET_WRITE ////////////
//////////// FILE OPERATIONS: IMPLEMENT TURRET_READ(?) ////////////


static int driver_entry(void)
{
    printk(KERN_ALERT "%s: loaded module\n", DEVICE_NAME);

    return 0;
}


static void driver_exit(void)
{
    printk(KERN_ALERT "%s: unloaded module\n", DEVICE_NAME);

    return;
}


module_init(driver_entry);
module_exit(driver_exit);

//////////////////////////////////////////////////////////////////////
///////////////////////// DRIVER INFORMATION /////////////////////////
//////////////////////////////////////////////////////////////////////
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph 'Makleford' Harkleroad");
MODULE_DESCRIPTION("Customized USB Missile Launcher driver (see: http://matthias.vallentin.net/blog/2007/04/writing-a-linux-kernel-driver-for-an-unknown-usb-device/)");
MODULE_VERSION("0.1");  // Not yet releasable
