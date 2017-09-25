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
#define BUFF_SIZE 9
#define MILLI_WAIT 10000
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))

int blink_open(struct inode *inode, struct file *filp);
int blink_close(struct inode *inode, struct file *filp);
ssize_t blink_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset);
static void blink_completion_handler(struct urb* urb);
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
    // struct semaphore sem;
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
int bytesTransferred;                       // usb_interrupt_msg() wants a place to store the actual bytes transferred
__le16 maxPacketSize;                       // wMaxPacketSize of the endpoint


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
    // retVal = down_interruptible(&virtual_device.sem);
    // if (retVal != 0)
    // {
    //     printk(KERN_ALERT "%s: could not lock device during open\n", DEVICE_NAME);
    //     return -1;
    // }
    // else
    // {
    //     printk(KERN_INFO "%s: opened device\n", DEVICE_NAME);
    // }
    printk(KERN_INFO "%s: opened device\n", DEVICE_NAME);

    return 0;
}


// Called upon user close
int blink_close(struct inode *inode, struct file *filp)
{
    // By calling up, which is opposite of down for semaphores, we release
    //  the mutex that we obtained at device open
    // This has the effect of allowing other processes to use the device now
    // up(&virtual_device.sem);
    printk(KERN_INFO "%s: closed device\n", DEVICE_NAME);
    return 0;
}


// Called when user wants to send information to the device
ssize_t blink_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
    // Send data from user to kernel
    // copy_from_user(dest, source, count)
    printk(KERN_INFO "%s: writing %lu bytes to device\n", DEVICE_NAME, bufCount);

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
            for (i = 0; i < BUFF_SIZE; i++)
            {
                printk(KERN_INFO "%s: Transfer Buffer[%d] == %d\n", DEVICE_NAME, i, virtual_device.transferBuff[i]);
            }

            printk(KERN_INFO "%s: Filling in the URB\n", DEVICE_NAME);
            /* URB ATTEMPT #1 IS FAILING */
            // usb_fill_int_urb(blinkURB, blinkDevice, blinkPipe,
            //                  virtual_device.transferBuff, MIN(bufCount, BUFF_SIZE),
            //                  (usb_complete_t)blink_completion_handler, blinkURB->context, blinkInterval);

            // Submit URB w/ int usb_submit_urb(struct urb *urb, int mem_flags);
            printk(KERN_INFO "%s: Submitting the URB\n", DEVICE_NAME);
            // retVal = usb_submit_urb(blinkURB, GFP_KERNEL);

            /* TIME FOR URB ATTEMPT #3 */
            // int usb_control_msg(struct usb_device *dev, unsigned int pipe,
            //                     _ _u8 request, _ _u8 requesttype,
            //                     _ _u16 value, _ _u16 index,
            //                     void *data, _ _u16 size, int timeout);
            retVal = usb_control_msg(blinkDevice, blinkPipe, 3, 2, 0, 0, virtual_device.transferBuff, maxPacketSize, MILLI_WAIT);
            // retVal = usb_interrupt_msg(blinkDevice, blinkPipe, virtual_device.transferBuff, MIN(bufCount, BUFF_SIZE), &bytesTransferred, MILLI_WAIT);
            printk(KERN_INFO "%s: USB control message returned: %d.\n", DEVICE_NAME, retVal);
            retVal = 0;

            /* TIME FOR URB ATTEMPT #2 */
            // int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe,
            // 	void *data, int len, int *actual_length, int timeout);
            retVal = usb_interrupt_msg(blinkDevice, blinkPipe, virtual_device.transferBuff, maxPacketSize, &bytesTransferred, MILLI_WAIT);
            // retVal = usb_interrupt_msg(blinkDevice, blinkPipe, virtual_device.transferBuff, MIN(bufCount, BUFF_SIZE), &bytesTransferred, MILLI_WAIT);
            printk(KERN_INFO "%s: URB interrupt message transferred %d bytes.\n", DEVICE_NAME, bytesTransferred);

            switch(retVal)
            {
                case(0):
                    printk(KERN_INFO "%s: URB successfully completed!\n", DEVICE_NAME);
                    break;
                case(-ENOENT):
                    printk(KERN_INFO "%s: URB successfully completed?\n", DEVICE_NAME);
                    printk(KERN_INFO "%s: The URB reported %s.\n", DEVICE_NAME, "The urb was stopped by a call to usb_kill_urb (-ENOENT)");
                    break;
                case(-ECONNRESET):
                    printk(KERN_INFO "%s: URB successfully completed?\n", DEVICE_NAME);
                    printk(KERN_INFO "%s: The URB reported %s.\n", DEVICE_NAME, "The urb was unlinked by a call to usb_unlink_urb (-ECONNRESET)");
                    break;
                case(-ESHUTDOWN):
                    printk(KERN_INFO "%s: URB successfully completed?\n", DEVICE_NAME);
                    printk(KERN_INFO "%s: The URB reported %s.\n", DEVICE_NAME, "There was a severe error with the USB host controller driver (-ESHUTDOWN)");
                    break;
                case(-EINPROGRESS):
                    printk(KERN_ALERT "%s: The URB returned %s!\n", DEVICE_NAME, "The urb is still being processed by the USB host controllers (-EINPROGRESS)");
                    break;
                case(-EPROTO):
                    printk(KERN_ALERT "%s: There may be hardware problems with the device!\n", DEVICE_NAME);
                    printk(KERN_ALERT "%s: The URB returned %s!\n", DEVICE_NAME, "A bitstuff error happened during transfer or no response packet was received by the hardware (-EPROTO)");
                    break;
                case(-EILSEQ):
                    printk(KERN_ALERT "%s: There may be hardware problems with the device!\n", DEVICE_NAME);
                    printk(KERN_ALERT "%s: The URB returned %s!\n", DEVICE_NAME, "There was a CRC mismatch in the urb transfer (-EILSEQ)");
                    break;
                case(-EPIPE):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Stalled endpoint (-EPIPE)");
                    break;
                case(-ECOMM):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Data was received too fast to be written to system memory (-ECOMM)");
                    break;
                case(-ENOSR):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Data retrieval was to slow for the USB data rate (-ENOSR)");
                    break;
                case(-EOVERFLOW):
                    printk(KERN_ALERT "%s: There may be hardware problems with the device!\n", DEVICE_NAME);
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "A 'babble' error happened to the URB (-EOVERFLOW)");
                    break;
                case(-EREMOTEIO):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Full amount of data was not received (-EREMOTEIO)");
                    break;
                case(-ENODEV):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Unplugged device (-ENODEV)");
                    break;
                case(-EXDEV):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Transfer was only partially completed (-EXDEV)");
                    break;
                case(-EINVAL):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Something very bad happened with the URB. Log off and go home. (-EINVAL)");
                    break;
                // These last three cases don't show up in Linux Device Drivers 3rd Edition but I found them somewhere
                case(-ENOMEM):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Out of memory (-ENOMEM)");
                    break;
                case(-EAGAIN):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Too many queued ISO transfers (-EAGAIN)");
                    break;
                case(-EFBIG):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Too many requested ISO frames (-EFBIG)");
                    break;
                case(-ETIMEDOUT):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Connection timed out (-ETIMEDOUT)");
                    break;
                default:
                    printk(KERN_ALERT "%s: URB returned an unknown error value of: %d!\n", DEVICE_NAME, retVal);
                    break;
            }
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


static void blink_completion_handler(struct urb* urb)
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
            switch(urb->status)
            {
                case(0):
                    printk(KERN_INFO "%s: URB completion handler successfully completed!\n", DEVICE_NAME);
                    break;
                case(-ENOENT):
                    printk(KERN_INFO "%s: URB completion handler successfully completed?\n", DEVICE_NAME);
                    printk(KERN_INFO "%s: The URB completion handler reported %s.\n", DEVICE_NAME, "The urb was stopped by a call to usb_kill_urb (-ENOENT)");
                    break;
                case(-ECONNRESET):
                    printk(KERN_INFO "%s: URB completion handler successfully completed?\n", DEVICE_NAME);
                    printk(KERN_INFO "%s: The URB completion handler reported %s.\n", DEVICE_NAME, "The urb was unlinked by a call to usb_unlink_urb (-ECONNRESET)");
                    break;
                case(-ESHUTDOWN):
                    printk(KERN_INFO "%s: URB completion handler successfully completed?\n", DEVICE_NAME);
                    printk(KERN_INFO "%s: The URB completion handler reported %s.\n", DEVICE_NAME, "There was a severe error with the USB host controller driver (-ESHUTDOWN)");
                    break;
                case(-EINPROGRESS):
                    printk(KERN_ALERT "%s: The URB completion handler returned %s!\n", DEVICE_NAME, "The urb is still being processed by the USB host controllers (-EINPROGRESS)");
                    break;
                case(-EPROTO):
                    printk(KERN_ALERT "%s: There may be hardware problems with the device!\n", DEVICE_NAME);
                    printk(KERN_ALERT "%s: The URB completion handler returned %s!\n", DEVICE_NAME, "A bitstuff error happened during transfer or no response packet was received by the hardware (-EPROTO)");
                    break;
                case(-EILSEQ):
                    printk(KERN_ALERT "%s: There may be hardware problems with the device!\n", DEVICE_NAME);
                    printk(KERN_ALERT "%s: The URB completion handler returned %s!\n", DEVICE_NAME, "There was a CRC mismatch in the urb transfer (-EILSEQ)");
                    break;
                case(-EPIPE):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Stalled endpoint (-EPIPE)");
                    break;
                case(-ECOMM):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Data was received too fast to be written to system memory (-ECOMM)");
                    break;
                case(-ENOSR):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Data retrieval was to slow for the USB data rate (-ENOSR)");
                    break;
                case(-EOVERFLOW):
                    printk(KERN_ALERT "%s: There may be hardware problems with the device!\n", DEVICE_NAME);
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "A 'babble' error happened to the URB (-EOVERFLOW)");
                    break;
                case(-EREMOTEIO):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Full amount of data was not received (-EREMOTEIO)");
                    break;
                case(-ENODEV):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Unplugged device (-ENODEV)");
                    break;
                case(-EXDEV):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Transfer was only partially completed (-EXDEV)");
                    break;
                case(-EINVAL):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Something very bad happened with the URB. Log off and go home. (-EINVAL)");
                    break;
                // These last three cases don't show up in Linux Device Drivers 3rd Edition but I found them somewhere
                case(-ENOMEM):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Out of memory (-ENOMEM)");
                    break;
                case(-EAGAIN):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Too many queued ISO transfers (-EAGAIN)");
                    break;
                case(-EFBIG):
                    printk(KERN_ALERT "%s: URB completion handler returned: %s!\n", DEVICE_NAME, "Too many requested ISO frames (-EFBIG)");
                    break;
                case(-ETIMEDOUT):
                    printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "Connection timed out (-ETIMEDOUT)");
                    break;
                default:
                    printk(KERN_ALERT "%s: URB completion handler returned an unknown error value of: %d!\n", DEVICE_NAME, retVal);
                    break;
            }
            break;
    }
}


/* DEVICE OPERATIONS */
static int blink_probe(struct usb_interface* interface, const struct usb_device_id* id)
{
    retVal = 0;

    blinkDevice = interface_to_usbdev(interface);

    /* ATTEMPTINT TO FIND ENDPOINT */
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
    }
    /* HARD CODED */
    // unsigned int usb_rcvintpipe(struct usb_device *dev, unsigned int endpoint)
    blinkPipe = usb_rcvintpipe(blinkDevice, interface->cur_altsetting->endpoint[0].desc.bEndpointAddress);
    blinkInterval = interface->cur_altsetting->endpoint[0].desc.bInterval;
    maxPacketSize = interface->cur_altsetting->endpoint[0].desc.wMaxPacketSize;
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
    // sema_init(&virtual_device.sem, 1);

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

/*
- Fade to RGB color       format: { 1, 'c', r,g,b,     th,tl, n } (*)
- Set RGB color now       format: { 1, 'n', r,g,b,       0,0, 0 }
- Read current RGB color  format: { 1, 'r', 0,0,0,       0,0, n } (2)
- Serverdown tickle/off   format: { 1, 'D', on,th,tl,  st,sp,ep } (*)
- Play/Pause              format: { 1, 'p', on,sp,0,     0,0, 0 }
- PlayLoop                format: { 1, 'p', on,sp,ep,c,    0, 0 } (2)
- Playstate readback      format: { 1, 'S', 0,0,0,       0,0, 0 } (2)
- Set color pattern line  format: { 1, 'P', r,g,b,     th,tl, p }
- read color pattern line format: { 1, 'R', 0,0,0,       0,0, p }
- Save color patterns     format: { 1, 'W', 0xBE,0xEF,0xCA,0xFE,0, 0 } (2)
- Read EEPROM location    format: { 1, 'e', ad,0,0,      0,0, 0 } (1)
- Write EEPROM location   format: { 1, 'E', ad,v,0,      0,0, 0 } (1)
- Get version             format: { 1, 'v', 0,0,0,       0,0, 0 }
- Test command            format: { 1, '!', 0,0,0,       0,0, 0 }
 */
