#include <linux/module.h>       // ALWAYS NEED
#include <linux/kernel.h>       // ALWAYS NEED
#include <linux/usb.h>          // Always needed for USB descriptors
// Multi-threading synchronization
#include <linux/semaphore.h>    // used to access semaphores, synchronization behaviors
// Used to assist in mapping data from user space to kernel space
#include <asm/uaccess.h>         // copy_to_user, copy_from_user

#define DEVICE_NAME "blink(1) device"
// Get these values from 'lsusb -v'
#define VENDOR_ID   0x27B8
#define PRODUCT_ID  0x01ED
#define BUFF_SIZE 8

int device_open(struct inode *inode, struct file *filp);
int device_close(struct inode *inode, struct file *filp);
static int blink_probe(struct usb_interface* interface, const struct usb_device_id* id);
static void blink_disconnect(struct usb_interface* interface);

////////////////////////////////
/* DEFINE NECESSARY VARIABLES */
////////////////////////////////
// 1. Create a struct for our fake device
struct fake_device
{
    char data[BUFF_SIZE + 1];  // Holds data from user to device
    struct semaphore sem;
} virtual_device;

// 2. Create a usb_device_id struct
// Describes type of USB devices this driver supports
// Adds VendorID and Product ID to the usb_device_id struct
static struct usb_device_id blink_table[] =
{
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {} // Terminating entry
};
MODULE_DEVICE_TABLE(usb, blink_table);

// 3. Create a USB Driver struct
static struct usb_driver blink_driver =
{
    .name = DEVICE_NAME,
    .id_table = blink_table,
    .probe = blink_probe,
    .disconnect = blink_disconnect,
};

// 4. USB Request Block pointer
struct urb* blinkURB = NULL;

// 5. Define file operations
// This tells the kernel which functions to call when a user operates on our device file_operations
struct file_operations blinkOps =
{
    .owner = THIS_MODULE,       // Prevent unloading of this module when operations are in use
    .open = device_open,        // Points to the method to call when opening the device
    .release = device_close,    // Points to the method to call when closing the device
    .write = device_write,      // Points to the method to call when writing to the device
    // .read = device_read         // Points to the method to call when reading from the device
};

// 6. Other
int retVal;             // Will be used to hold return values of functions; this is because the kernel stack is very small
                        //  so declaring variables all over the place in our module functions eats up the stack very fast


/////////////////////////////////////
/* USB DEVICE DRIVER FUNCTIONALITY */
/////////////////////////////////////

/* FILE OPERATIONS */
//  Called on defice_file open
//      inode reference to the file on disk
//      and contains information about that file
//      struct file represents an abstract open file
int device_open(struct inode *inode, struct file *filp)
{
    // Only allow one process to open this device by using a semaphore as mutal exclusive lock - mutext
    retVal = down_interruptible(&virtual_device.sem);
    if (retVal != 0)
    {
        printk(KERN_ALERT "%s: could not lock device during open\n", DEVICE_NAME);
        return -1;
    }
    else
    {
        printk(KERN_INFO "%s: opened device\n", DEVICE_NAME);
    }

    return 0;
}


// Called upon user close
int device_close(struct inode *inode, struct file *filp)
{
    // By calling up, which is opposite of down for semaphores, we release
    //  the mutex that we obtained at device open
    // This has the effect of allowing other processes to use the device now
    up(&virtual_device.sem);
    printk(KERN_INFO "%s: closed device\n", DEVICE_NAME);
    return 0;
}


// Called when user wants to send information to the device
ssize_t blink_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
    // Send data from user to kernel
    // copy_from_user(dest, source, count)
    printk(KERN_INFO "%s: writing to device\n", DEVICE_NAME);
    return copy_from_user(virtual_device.data, bufSourceData, BUFF_SIZE);
}


/* DEVICE OPERATIONS */
static int blink_probe(struct usb_interface* interface, const struct usb_device_id* id)
{
    printk(KERN_INFO "%s: blink(1) (%04X:%04X) active\n", DEVICE_NAME, id->idVendor, id->idProduct);
    return 0;
}


static void blink_disconnect(struct usb_interface* interface)
{
    printk(KERN_INFO "%s: blink(1) removed\n", DEVICE_NAME);
    return;
}


static int __init driver_entry(void)
{
    // 1. Initialize the semaphore
    sema_init(&virtual_device.sem, 1);

    // 2. Register this driver with the USB subsystem
    retVal = usb_register(&blink_driver);
    if (retVal)
    {
        printk(KERN_ALERT "%s: Unable to register USB\nusb_register() returned %d\n", DEVICE_NAME, retVal);
    }

    // 3. Allocate an URB to use
    blinkURB = usb_alloc_urb(0, 0);

    return retVal;
}


static void __exit driver_exit(void)
{
    usb_deregister(&blink_driver);
    printk(KERN_ALERT "%s: unloaded module\n", DEVICE_NAME);

    return;
}

module_init(driver_entry);
module_exit(driver_exit);


////////////////////////
/* DRIVER INFORMATION */
////////////////////////
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph 'Makleford' Harkleroad");
MODULE_DESCRIPTION("Customized blink(1) driver (see: https://blink1.thingm.com/)");
MODULE_VERSION("0.1");  // Not yet releasable

//////////////////////////
/* GATHERED INFORMATION */
//////////////////////////

/* COMMAND CODES */
// 'c';   // command code for 'fade to rgb'
// 'n';   // command code for "set rgb now"
// 'W';   // command code for "write pattern to flash"
// 'l';   // command code for "set ledn"
// '!';   // command code for "test"
// 'v';   // command code for "get version"
// 'r';   // command code for "read rgb"
// 'S';   // command code for "read play state"
// 'P';   // command code for "write pattern line"
// 'R';   // command code for "read pattern line n"
