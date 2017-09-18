#include <linux/module.h>       // ALWAYS NEED
#include <linux/kernel.h>       // ALWAYS NEED
// Multi-threading synchronization
#include <linux/semaphore.h>    // used to access semaphores, synchronization behaviors


// 1. Create a struct for our fake device
struct fake_device
{
    char data[100];
    struct semaphore sem;
} virtual_device;


#define DEVICE_NAME     "blinkDevice"

// 7. Define the file operation functions
// 7.1. device_open
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
        printk(KERN_ALERT "myTestDevice: could not lock device during open\n");
        return -1;
    }
    else
    {
        printk(KERN_INFO "myTestDevice: opened device\n");
    }

    return 0;
}


// 7.2. device_read
// Called when user wants to get information from the device
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset)
{
    // Take data from kernel space (device) to user space (process)
    printk(KERN_INFO "myTestDevice: reading from device\n");
    return copy_to_user(bufStoreData, virtual_device.data, bufCount);
}


// 7.3. device_write
// Called when user wants to send information to the device
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
    // Send data from user to kernel
    // copy_from_user(dest, source, count)
    printk(KERN_INFO "myTestDevice: writing to device\n");
    return copy_from_user(virtual_device.data, bufSourceData, bufCount);
}


// 7.4. device_close
// Called upon user close
int device_close(struct inode *inode, struct file *filp)
{
    // By calling up, which is opposite of down for semaphores, we release
    //  the mutex that we obtained at device open
    // This has the effect of allowing other processes to use the device now
    up(&virtual_device.sem);
    printk(KERN_INFO "myTestDevice: closed device\n");
    return 0;
}


// 6. Define the file_operations struct
// This tells the kernel which functions to call when a user operates on our device file_operations
struct file_operations fops =
{
    .owner = THIS_MODULE,       // Prevent unloading of this module when operations are in use
    .open = device_open,        // Points to the method to call when opening the device
    .release = device_close,    // Points to the method to call when closing the device
    .write = device_write,      // Points to the method to call when writing to the device
    .read = device_read         // Points to the method to call when reading from the device
};

// int driver_entry(void)
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
    // printk(KERN_INFO "\tDEVICE_NAME: %s\n", DEVICE_NAME);
    // printk(KERN_INFO "\tMAJOR NUMBER: %d\n", major_number);
    printk(KERN_INFO "\tUse 'mknod /dev/%s c %d 0' for device file\n", DEVICE_NAME, major_number);

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
        // PRO TIP:  USYNC_THREAD and USYNC_PROCESS come from synch.h on Solaris
        // retVal = sema_init(&virtual_device.sem, USYNC_THREAD, NULL);
        // retVal = sema_init(&virtual_device.sem, USYNC_PROCESS, NULL);
        // retVal = sema_init(&virtual_device.sem, 1, NULL);  // ERROR: TOO MANY ARGUMENTS
        sema_init(&virtual_device.sem, 1);

        // PRO TIP:  Non-Solaris sema_init has a return value of void
        //     if (retVal != 0)
        //     {
        //         printk(KERN_ALERT "myTestDevice: unable to initialize a semaphore\n");
        //         return retVal;
        //     }
        // }
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
