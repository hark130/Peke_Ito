#include <linux/module.h>       // ALWAYS NEED
#include <linux/kernel.h>       // ALWAYS NEED
#include <linux/slab.h>         // kmalloc
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
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))

int blink_open(struct inode *inode, struct file *filp);
int blink_close(struct inode *inode, struct file *filp);
ssize_t blink_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset);
static void blink_completion_handler(struct urb urb);
static int blink_probe(struct usb_interface* interface, const struct usb_device_id* id);
static void blink_disconnect(struct usb_interface* interface);

////////////////////////////////
/* DEFINE NECESSARY VARIABLES */
////////////////////////////////
// 1. Create a struct for our fake device
struct fake_device
{
    // char data[BUFF_SIZE + 1];   // Holds data from user to device
    char* transferBuff;         // Allocate memory here to transfer data within the URB
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
    .open = blink_open,        // Points to the method to call when opening the device
    .release = blink_close,    // Points to the method to call when closing the device
    .write = blink_write,      // Points to the method to call when writing to the device
    // .read = blink_read         // Points to the method to call when reading from the device
};

// 6. Other
static struct usb_device* blinkDevice;      // Initialized by interface_to_usbdev(interface) in blink_probe()
static struct usb_class_driver blinkClass;  // ???
int major_number;                           // Will store our major number - extracted from dev_t using macro - mknod /director/file c major minor
int retVal;                                 // Will be used to hold return values of functions; this is because the kernel stack is very small
                                            //  so declaring variables all over the place in our module functions eats up the stack very fast
int i;                                      // Iterating variable
unsigned int blinkPipe;                     // The specific endpoint of the USB device to send URBs
unsigned int blinkInterval;                 // Interval for polling endpoint for data transfers


/////////////////////////////////////
/* USB DEVICE DRIVER FUNCTIONALITY */
/////////////////////////////////////

/* FILE OPERATIONS */
//  Called on defice_file open
//      inode reference to the file on disk
//      and contains information about that file
//      struct file represents an abstract open file
int blink_open(struct inode *inode, struct file *filp)
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
int blink_close(struct inode *inode, struct file *filp)
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

    // Allocate a transfer buffer
    virtual_device.transferBuff = (char*)kmalloc(BUFF_SIZE, GFP_KERNEL);
    if(virtual_device.transferBuff == NULL)
    {
        printk(KERN_ERR "%s: Unable to allocate kernel memory for transferBuff!\n", DEVICE_NAME);
        retVal = -ENOMEM;
    }
    else
    {
        // Zeroize the transfer buffer
        for (i = 0; i < BUFF_SIZE; i++)
        {
            virtual_device.transferBuff[i] = 0;
        }

        // Read data from user space
        if((retVal = copy_from_user(virtual_device.transferBuff, bufSourceData, MIN(bufCount, BUFF_SIZE))) != 0)
        {
            retVal = -EFAULT;
        }
        else
        {
            /*
            // unsigned int usb_rcvintpipe(struct usb_device *dev, unsigned int endpoint)
            usb_fill_int_urb(blinkURB, blinkDevice, blinkPipe,
                             virtual_device.transferBuff, MIN(bufCount, BUFF_SIZE),
                             blink_completion_handler,
                             blinkURB->context,
                             blinkInterval };
                             */
            printk(KERN_INFO "%s: This is where we would be filling in the URB\n", DEVICE_NAME);

            // Submit URB w/ int usb_submit_urb(struct urb *urb, int mem_flags);
            printk(KERN_INFO "%s: This is where we would be submitting the URB\n", DEVICE_NAME);
        }
    }


    // 1. Use the URB to write the data to the device
    // 2. Test the device by sending hard-coded static data to it
    // 3. Test the device by programatically sending data to it

    /*
    // Write the data into the bulk endpoint
    retval = usb_bulk_msg(device, usb_sndbulkpipe(device, BULK_EP_OUT),
            bulk_buf, MIN(cnt, MAX_PKT_SIZE), &wrote_cnt, 5000);
    if (retval)
    {
        printk(KERN_ERR "Bulk message returned %d\n", retval);
        return retval;
    }

    return wrote_cnt;
    */

    return retVal;
}


static void blink_completion_handler(struct urb urb)
/* called when data arrives from device (usb-core)*/
{
    // struct foo *foo = (struct foo *)urb->context;
    // unsigned char* data = foo->data;  /* the data from the device */
    // struct input_dev *input_dev = foo->inputdev;
    switch(urb->status)
    {
        case 0:
            /* success, first process data, then send keys, abs/rel, events */
            // input_report_abs(input_dev, type, code, value);  // NOT SURE WHAT THIS DOES
            /* and/or input_event(), input_report_rel(), input_report_key() */
            printk(KERN_INFO "%s: The completion handler was successful.\n", DEVICE_NAME);
            break;
        default:
            /* handle error */
            printk(KERN_ERR "%s: The completion handler experienced an error status of %d.\n", DEVICE_NAME, urb->status);
            break;
    }
}


/* DEVICE OPERATIONS */
static int blink_probe(struct usb_interface* interface, const struct usb_device_id* id)
{
    retVal = 0;

    blinkDevice = interface_to_usbdev(interface);

    /* ATTEMPTINT TO FIND ENDPOINT */
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
    }
    /* HARD CODED */
    blinkPipe = usb_rcvintpipe(blinkDevice, interface->cur_altsetting->endpoint[0].desc.bEndpointAddress);
    blinkInterval = interface->cur_altsetting->endpoint[0].desc.bEndpointInterval;
    retVal = 0;  // Done messing around with DEBUG statements

    blinkClass.name = "usb/blink%d";
    blinkClass.fops = &blinkOps;
    if ((retVal = usb_register_dev(interface, &blinkClass)) < 0)
    {
        /* Something prevented us from registering this driver */
        printk(KERN_ALERT "%s: Not able to get a minor.\n", DEVICE_NAME);
    }
    else
    {
        printk(KERN_INFO "%s: Minor obtained: %d\n", DEVICE_NAME, interface->minor);
    }

    printk(KERN_INFO "%s: blink(1) (%04X:%04X) active\n", DEVICE_NAME, id->idVendor, id->idProduct);
    return retVal;
}


static void blink_disconnect(struct usb_interface* interface)
{
    usb_deregister_dev(interface, &blinkClass);
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

    // 4. Zeroize data
    // for (i = 0; i <= BUFF_SIZE; i++)
    // {
    //     virtual_device.data[i] = 0;
    // }

    return retVal;
}


static void __exit driver_exit(void)
{
    // Deallocate the URB
    usb_free_urb(blinkURB);
    blinkURB = NULL;
    // Unregister the device driver
    usb_deregister(&blink_driver);
    // Log it
    printk(KERN_INFO "%s: unloaded module\n", DEVICE_NAME);

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
