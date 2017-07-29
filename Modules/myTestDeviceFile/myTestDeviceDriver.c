// Command used to create device file...
// sudo mknod /dev/myTestDeviceFile c 1337 0

#include <linux/module.h>       // ALWAYS NEED
#include <linux/kernel.h>       // ALWAYS NEED
// Used to allow file operations
#include <linux/fs.h>           // file_operations structure - allows use to open/close, read/write to device
// Used to help register your char device to the kernel
#include <linux/cdev.h>         // This is a char driver, makes cdev available
// Multi-threading synchronization
#include <linux/semaphore.h>    // used to access semaphores, synchronization behaviors
// Used to assist in mapping data from user space to kernel space
#include <asm/uacess.h>         // copy_to_user, copy_from_user


// 1. Create a struct for our fake device
struct fake_device
{
    char data[100];
    struct semaphore sem;
} virtual_device;

// 2. To later regiser our device we need a cdev object and some other variables
struct cdev *myCdev;    // My character device driver
int major_number;       // Will store our major number - extracted from dev_t using macro - mknod /director/file c major minor
int retVal;             // Will be used to hold return values of functions; this is because the kernel stack is very small
                        //  so declaring variables all over the place in our module functions eats up the stack very fast
dev_t dev_num;          // Will hold major number that kernel gives us
                        //  name--> appears in /proc/devices
#define DEVICE_NAME     "myTestDevice"


static int driver_entry(void)
{
    // 3. Register our device with the system
    // 3.1. Use dynamic allocation to assign our device
    //      a major number-- alloc_chrdev_region(dev_t*, uint fminor, uint count, char* name)
    retVal = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (retVal < 0)
    {
        printk(KERN_ALERT "myTestDevice: failed to allocate a major number\n");
        return retVal;
    }
    major_number = MAJOR(dev_num);  // Macro extracts the major number
    printk(KERN_INFO "myTestDevice: major number is %d\n", major_number);
    printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file", DEVICE_NAME, major_number);

    // 3.2. Allocate and initialize the character device structure
    myCdev = cdev_alloc();  // Create our cdev structure
    // initialize our cdev structure
    if (myCdev)
    {
        myCdev->ops = &fops;            // Struct file_operations
        myCdev->owner = THIS_MODULE;    // Very common
    }
    else
    {
        printk(KERN_ALERT "myTestDevice: failed to allocate the cdev structure\n");
        return -1;
    }

    // Now add this character device to the system
    retVal = cdev_add(myCdev, dev_num, 1);

    if (retVal < 0)
    {
        printk(KERN_ALERT "myTestDevice: unable to add cdev to kernel\n");
        return retVal;
    }
    // The device is now "live" and can be called by the kernel
    else
    {
        // 4. Initialize the semaphore
        sema_init(&virtual_device.sem, 1);
    }

    return 0;
}


static void driver_exit(void)
{
    // 5. Do everything in the reverse order
    // 5.1. Unregister the cdev
    cdev_del(myCdev);

    // 5.2. Unregister the device number
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_ALERT "myTestDevice: unloaded module\n");

    return;
}

module_init(driver_entry);
module_exit(driver_exit);
