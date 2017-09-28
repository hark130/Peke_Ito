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

/* SETUP PACKET MACROS */
/* bmRequestTypes */
// Recipient
#define SP_RT_RCPT_DEVICE 0x0       // Device
#define SP_RT_RCPT_INTRFC 0x1       // Interface
#define SP_RT_RCPT_ENDPNT 0x2       // Endpoint
#define SP_RT_RCPT_OTHER 0x3        // Other
// Type
#define SP_RT_TYPE_STNRD 0x0       // Standard
#define SP_RT_TYPE_CLASS 0x1 << 5  // Class
#define SP_RT_TYPE_VENDR 0x2 << 5  // Vendor
#define SP_RT_TYPE_RSVRD 0x3 << 5  // Reserved
// Data Phase Transfer Direction
#define SP_RT_DPTD_H2D 0x0          // Host to Device
#define SP_RT_DPTD_D2H 0x1 << 7     // Device to Host
/* bRequest */
#define SP_RQST_GET_STATUS 0x0
#define SP_RQST_CLEAR_FEATURE 0x1
#define SP_RQST_SET_FEATURE 0x3
#define SP_RQST_SET_ADDRESS 0x5
#define SP_RQST_GET_DESCRIPTOR 0x6
#define SP_RQST_SET_DESCRIPTOR 0x7
#define SP_RQST_GET_CONFIG 0x8
#define SP_RQST_SET_CONFIG 0x9
#define SP_RQST_GET_INTERFACE 0xA
#define SP_RQST_SET_INTERFACE 0xB
#define SP_RQST_SYNCH_FRAME 0xC


int blink_open(struct inode *inode, struct file *filp);
int blink_close(struct inode *inode, struct file *filp);
// ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
ssize_t blink_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset);
// ssize_t (*read) (struct file *, char *, size_t, loff_t *);
ssize_t blink_read(struct file* filp, char* bufSourceData, size_t bufCount, loff_t* curOffset);
static void blink_completion_handler(struct urb* urb);
static int blink_probe(struct usb_interface* interface, const struct usb_device_id* id);
static void blink_disconnect(struct usb_interface* interface);
void log_return_value(char* functionName, int retVal);

////////////////////////////////
/* DEFINE NECESSARY VARIABLES */
////////////////////////////////
// 1. Create a struct for our fake device
struct fake_device
{
    // char data[BUFF_SIZE + 1];   // Holds data from user to device
    char* transferBuff;         // Allocate memory here to transfer data within the URB
    char setupPacket[8];
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
struct urb* blinkURB_Control = NULL;

// 5. Define file operations
// This tells the kernel which functions to call when a user operates on our device file_operations
struct file_operations blinkOps =
{
    .owner = THIS_MODULE,      // Prevent unloading of this module when operations are in use
    .open = blink_open,        // Points to the method to call when opening the device
    .release = blink_close,    // Points to the method to call when closing the device
    .write = blink_write,      // Points to the method to call when writing to the device
    .read = blink_read,        // Points to the method to call when reading from the device
};

// 6. Other
static struct usb_device* blinkDevice;      // Initialized by interface_to_usbdev(interface) in blink_probe()
static struct usb_class_driver blinkClass;  // ???
int major_number;                           // Will store our major number - extracted from dev_t using macro - mknod /director/file c major minor
int retVal;                                 // Will be used to hold return values of functions; this is because the kernel stack is very small
                                            //  so declaring variables all over the place in our module functions eats up the stack very fast
int i;                                      // Iterating variable
unsigned int blinkPipeIntRecv;              // The specific endpoint of the USB device to send Interrupt URBs
unsigned int blinkPipeIntSend;              // The specific endpoint of the USB device to recv Interrupt URBs
unsigned int blinkPipe_Control;             // The specific endpoint of the USB device to send Control URBs
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
        if((retVal = copy_from_user(virtual_device.transferBuff, bufSourceData, bufCount)) != 0)
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
            /* ATTTEMPT #4... Send a URB and then a HID SET_REPORT */
            // void usb_fill_int_urb(struct urb *urb,
            // 				      struct usb_device *dev,
            // 			  	      unsigned int pipe,
            // 				      void *transfer_buffer,
            // 				      int buffer_length,
            // 				      usb_complete_t complete_fn,
            // 				      void *context,
            // 				      int interval)
            printk(KERN_INFO "%s: usb_fill_int_urb - Using pipe #%d.\n", DEVICE_NAME, blinkPipeIntRecv);
            usb_fill_int_urb(blinkURB, blinkDevice, blinkPipeIntRecv,
                             NULL, 0, (usb_complete_t)blink_completion_handler,
                             blinkURB->context, blinkInterval);
            // Submit URB w/ int usb_submit_urb(struct urb *urb, int mem_flags);
            printk(KERN_INFO "%s: Submitting the URB\n", DEVICE_NAME);
            blinkURB->transfer_flags |= 0x204;  // Attempting to replicate the exact transfer_flags 
            retVal = usb_submit_urb(blinkURB, GFP_KERNEL);

            /* ATTEMPT #4.4 */
            // int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe,
	        //                       void *data, int len, int *actual_length, int timeout);
            // retVal = usb_interrupt_msg(blinkDevice, blinkPipeIntRecv,
            //                            NULL, 0, &bytesTransferred, MILLI_WAIT);

            log_return_value("blink_write - INT URB", retVal);

            /* ATTEMPT #4.1 */
            // int usb_control_msg(struct usb_device *dev, unsigned int pipe,
            //                     _ _u8 request, _ _u8 requesttype,
            //                     _ _u16 value, _ _u16 index,
            //                     void *data, _ _u16 size, int timeout);
            //  retVal = usb_control_msg(blinkDevice, blinkPipeIntRecv,
            //                           9, 33,
            //                           0x0301, 0,
            //                           virtual_device.transferBuff, bufCount, MILLI_WAIT);
                                      // NOTE: 0x0301 == __u16 value == 0000 0011 0000 0001
                                      //                                Rprt Type Report ID
                                      //                                3         1
            /* ATTEMPT #4.2 */
            // void usb_fill_control_urb(struct urb *urb,
            //     				      struct usb_device *dev,
            //     				      unsigned int pipe,
            //     					  unsigned char *setup_packet,
            //     					  void *transfer_buffer,
            //     					  int buffer_length,
            //     					  usb_complete_t complete_fn,
            //     					  void *context)
            // Create setupPacket
            // virtual_device.setupPacket[0] = SP_RT_DPTD_H2D | SP_RT_TYPE_CLASS | SP_RT_RCPT_INTRFC;  // bmRequestType
            // virtual_device.setupPacket[1] = SP_RQST_SET_CONFIG;  // bRequest
            // virtual_device.setupPacket[2] = 1; // wValue ReportID
            // virtual_device.setupPacket[3] = 3; // wValue ReportType
            // virtual_device.setupPacket[4] = 0; // wIndex
            // virtual_device.setupPacket[5] = 0; // wIndex
            // virtual_device.setupPacket[6] = 9; // wLength
            // virtual_device.setupPacket[7] = 0; // wLength
            // for (i = 0; i < 8; i++)
            // {
            //     printk(KERN_INFO "%s: Setup Packet Buffer[%d] == %d (0x%X)\n", DEVICE_NAME, i, virtual_device.setupPacket[i], virtual_device.setupPacket[i]);
            // }
            // printk(KERN_INFO "%s: usb_fill_control_urb - Using pipe #%d.\n", DEVICE_NAME, blinkPipe_Control);
            // usb_fill_control_urb(blinkURB_Control, blinkDevice, blinkPipe_Control,
            //                      virtual_device.setupPacket, virtual_device.transferBuff, bufCount,
            //                      (usb_complete_t)blink_completion_handler, blinkURB_Control->context);
            // // retVal = usb_submit_urb(blinkURB_Control, 0x204);
            // retVal = usb_submit_urb(blinkURB_Control, GFP_KERNEL);
            // log_return_value("blink_write - usb_fill_control_urb", retVal);

            /* ATTEMPT #4.3 */
            // int usb_control_msg(struct usb_device *dev, unsigned int pipe,
            //                     __u8 request, __u8 requesttype,
            //                     __u16 value, __u16 index,
            //                     void *data, __u16 size, int timeout);
            // If successful, the number of bytes transferred. Otherwise, a negative error number.
            retVal = usb_control_msg(blinkDevice, blinkPipe_Control,
                                     SP_RQST_SET_CONFIG, SP_RT_DPTD_H2D | SP_RT_TYPE_CLASS | SP_RT_RCPT_INTRFC,
                                     0x301, 0,
                                     virtual_device.transferBuff, bufCount, MILLI_WAIT);

            if (retVal == bufCount)
            {
                printk(KERN_INFO "%s: usb_control_msg successfully transferred %d bytes.\n", DEVICE_NAME, retVal);
                retVal = 0;  // Success
            }
            else if (retVal == 0)
            {
                retVal = -EINVAL;
            }

            log_return_value("blink_write - usb_control_msg", retVal);

            /* URB ATTEMPT #1 IS FAILING */
            // usb_fill_int_urb(blinkURB, blinkDevice, blinkPipeIntRecv,
            //                  virtual_device.transferBuff, MIN(bufCount, BUFF_SIZE),
            //                  (usb_complete_t)blink_completion_handler, blinkURB->context, blinkInterval);

            // // Submit URB w/ int usb_submit_urb(struct urb *urb, int mem_flags);
            // printk(KERN_INFO "%s: Submitting the URB\n", DEVICE_NAME);
            // retVal = usb_submit_urb(blinkURB, GFP_KERNEL);
            // log_return_value("blink_write", retVal);
            /* TIME FOR URB ATTEMPT #3 */
            // int usb_control_msg(struct usb_device *dev, unsigned int pipe,
            //                     _ _u8 request, _ _u8 requesttype,
            //                     _ _u16 value, _ _u16 index,
            //                     void *data, _ _u16 size, int timeout);
            // retVal = usb_control_msg(blinkDevice, blinkPipeIntRecv, 2, 3, 0, 0, virtual_device.transferBuff, maxPacketSize, MILLI_WAIT);
            // retVal = usb_control_msg(blinkDevice, blinkPipeIntRecv, 3, 2, 0, 0, virtual_device.transferBuff, maxPacketSize, MILLI_WAIT);
            // retVal = usb_control_msg(blinkDevice, blinkPipeIntRecv, 3, 2, 0, 0, virtual_device.transferBuff, 8, MILLI_WAIT);
            // retVal = usb_interrupt_msg(blinkDevice, blinkPipeIntRecv, virtual_device.transferBuff, MIN(bufCount, BUFF_SIZE), &bytesTransferred, MILLI_WAIT);
            // printk(KERN_INFO "%s: USB control message returned: %d.\n", DEVICE_NAME, retVal);
            // retVal = 0;

            /* TIME FOR URB ATTEMPT #2 */
            // int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe,
            // 	void *data, int len, int *actual_length, int timeout);
            // retVal = usb_interrupt_msg(blinkDevice, blinkPipeIntRecv, virtual_device.transferBuff, maxPacketSize, &bytesTransferred, MILLI_WAIT);
            // retVal = usb_interrupt_msg(blinkDevice, blinkPipeIntRecv, virtual_device.transferBuff, MIN(bufCount, BUFF_SIZE), &bytesTransferred, MILLI_WAIT);
            // printk(KERN_INFO "%s: URB interrupt message wrote %d bytes.\n", DEVICE_NAME, bytesTransferred);
        }
    }

    log_return_value("blink_write", retVal);

    // Free kernel malloc'd memory
    if(virtual_device.transferBuff)
    {
        printk(KERN_ERR "%s: blink_write() Freeing transferBuff kernel memory.\n", DEVICE_NAME);
        kfree(virtual_device.transferBuff);
        virtual_device.transferBuff = NULL;
    }

    return retVal;
}


// Called when user wants to read information from the device
ssize_t blink_read(struct file* filp, char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
	retVal = 0;
	printk(KERN_ERR "%s: Entering blink_read().\n", DEVICE_NAME);
	bytesTransferred = 0;  // Zeroize temp variable

    // Allocate a transfer buffer
    virtual_device.transferBuff = (char*)kmalloc(bufCount + 1, GFP_KERNEL);
    if(virtual_device.transferBuff == NULL)
    {
        printk(KERN_ERR "%s: blink_read() Unable to allocate kernel memory for transferBuff!\n", DEVICE_NAME);
        retVal = -ENOMEM;
    }
    else
    {
		/* Read the data from the interrupt endpoint */
		// int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe,
		//                       void *data, int len, int *actual_length, int timeout);
		retVal = usb_interrupt_msg(blinkDevice, blinkPipeIntSend,
								   virtual_device.transferBuff, maxPacketSize, &bytesTransferred, MILLI_WAIT);
		printk(KERN_INFO "%s: URB interrupt message says incoming bufCount %lu bytes.\n", DEVICE_NAME, bufCount);
		printk(KERN_INFO "%s: URB interrupt message read %d bytes.\n", DEVICE_NAME, bytesTransferred);
		log_return_value("blink_read - INT URB", retVal);

		if (virtual_device.transferBuff)
		{
            for (i = 0; i < bufCount; i++)
            {
                printk(KERN_INFO "%s: Read Buffer[%d] == %d(0x%X) char(%c)\n", DEVICE_NAME, i, \
					                                                           virtual_device.transferBuff[i], \
					                                                           virtual_device.transferBuff[i], \
					                                                           virtual_device.transferBuff[i]);
            }
		}
	}

    // Free kernel malloc'd memory
    if(virtual_device.transferBuff)
    {
        printk(KERN_ERR "%s: blink_read() Freeing transferBuff kernel memory.\n", DEVICE_NAME);
        kfree(virtual_device.transferBuff);
        virtual_device.transferBuff = NULL;
    }

	return retVal;
}


static void blink_completion_handler(struct urb* urb)
/* called when data arrives from device (usb-core)*/
{
    // Individual frame descriptor status fields may report more status codes.
    log_return_value("blink_completion_handler", urb->status);
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
    blinkPipeIntRecv = usb_rcvintpipe(blinkDevice, interface->cur_altsetting->endpoint[0].desc.bEndpointAddress);
    // unsigned int usb_rcvctrlpipe(struct usb_device *dev, unsigned int endpoint)
    blinkPipe_Control = usb_rcvctrlpipe(blinkDevice, 0);
    // blinkPipe_Control = usb_rcvctrlpipe(blinkDevice, interface->cur_altsetting->endpoint[0].desc.bEndpointAddress);
	// unsigned int usb_sndintpipe(struct usb_device *dev, unsigned int endpoint)
	blinkPipeIntSend = usb_sndintpipe(blinkDevice, interface->cur_altsetting->endpoint[0].desc.bEndpointAddress);
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


void log_return_value(char* functionName, int retVal)
{
    char* funcName = NULL;
    char blankString[1] = { 0 };

    if (!functionName)
    {
        funcName = blankString;
    }
    else
    {
        funcName = functionName;
    }

    switch(retVal)
    {
        case(0):
            printk(KERN_INFO "%s: %s URB successfully completed!\n", DEVICE_NAME, funcName);
            break;
        case(-ENOENT):
            printk(KERN_INFO "%s: %s URB successfully completed?\n", DEVICE_NAME, funcName);
            printk(KERN_INFO "%s: The URB reported %s.\n", DEVICE_NAME, "The urb was stopped by a call to usb_kill_urb (-ENOENT)");
            break;
        case(-ECONNRESET):
            printk(KERN_INFO "%s: %s URB successfully completed?\n", DEVICE_NAME, funcName);
            printk(KERN_INFO "%s: The URB reported %s.\n", DEVICE_NAME, "The urb was unlinked by a call to usb_unlink_urb (-ECONNRESET)");
            break;
        case(-ESHUTDOWN):
            printk(KERN_INFO "%s: %s URB successfully completed?\n", DEVICE_NAME, funcName);
            printk(KERN_INFO "%s: The URB reported %s.\n", DEVICE_NAME, "There was a severe error with the USB host controller driver (-ESHUTDOWN)");
            break;
        case(-EINPROGRESS):
            printk(KERN_ALERT "%s: %s The URB returned %s!\n", DEVICE_NAME, funcName, "The urb is still being processed by the USB host controllers (-EINPROGRESS)");
            break;
        case(-EPROTO):
            printk(KERN_ALERT "%s: %s There may be hardware problems with the device!\n", DEVICE_NAME, funcName);
            printk(KERN_ALERT "%s: The URB returned %s!\n", DEVICE_NAME, "A bitstuff error happened during transfer or no response packet was received by the hardware (-EPROTO)");
            break;
        case(-EILSEQ):
            printk(KERN_ALERT "%s: %s There may be hardware problems with the device!\n", DEVICE_NAME, funcName);
            printk(KERN_ALERT "%s: The URB returned %s!\n", DEVICE_NAME, "There was a CRC mismatch in the urb transfer (-EILSEQ)");
            break;
        case(-EPIPE):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Stalled endpoint (-EPIPE)");
            break;
        case(-ECOMM):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Data was received too fast to be written to system memory (-ECOMM)");
            break;
        case(-ENOSR):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Data retrieval was to slow for the USB data rate (-ENOSR)");
            break;
        case(-EOVERFLOW):
            printk(KERN_ALERT "%s: %s There may be hardware problems with the device!\n", DEVICE_NAME, funcName);
            printk(KERN_ALERT "%s: URB returned: %s!\n", DEVICE_NAME, "A 'babble' error happened to the URB (-EOVERFLOW)");
            break;
        case(-EREMOTEIO):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Full amount of data was not received (-EREMOTEIO)");
            break;
        case(-ENODEV):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Unplugged device (-ENODEV)");
            break;
        case(-EXDEV):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Transfer was only partially completed (-EXDEV)");
            break;
        case(-EINVAL):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Something very bad happened with the URB. Log off and go home. (-EINVAL)");
            break;
        // These last three cases don't show up in Linux Device Drivers 3rd Edition but I found them somewhere
        case(-ENOMEM):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Out of memory (-ENOMEM)");
            break;
        case(-EAGAIN):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Too many queued ISO transfers (-EAGAIN)");
            break;
        case(-EFBIG):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Too many requested ISO frames (-EFBIG)");
            break;
        case(-ETIMEDOUT):
            printk(KERN_ALERT "%s: %s URB returned: %s!\n", DEVICE_NAME, funcName, "Connection timed out (-ETIMEDOUT)");
            break;
        default:
            printk(KERN_ALERT "%s: %s URB returned an unknown error value of: %d!\n", DEVICE_NAME, funcName, retVal);
            break;
    }
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
    blinkURB_Control = usb_alloc_urb(0, 0);

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
    usb_free_urb(blinkURB_Control);
    blinkURB_Control = NULL;
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
