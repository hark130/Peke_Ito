/*
 *    PURPOSE - A block device driver for a virtual block device of my own creation
 */

/////////////
/* HEADERS */
/////////////
#include <linux/fs.h>                           // Defines file table structures
#include <linux/genhd.h>                        // Generic hard disk header file
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED

////////////
/* MACROS */
////////////
#define DEVICE_NAME "Virtual Block Device 0"    // virtBlockDev0
#define DEV_MAJOR_NUM 0                         // Major number
#define DEV_MAX_MINORS 1                        // The maximum number of minor numbers that this disk can have
#define HARKLE_KERROR(module, funcName, errNum) do { printk(KERN_ERR "%s: <<<ERROR>>> %s() returned %d!\n", module, #funcName, errNum); } while (0);
#ifndef DEV_TOTAL_SIZE
#define DEV_TOTAL_SIZE 100                      // 10M * 10
#endif  // DEV_TOTAL_SIZE

//////////////
/* TYPEDEFS */
//////////////
typedef struct _block_dev
{
    struct request_queue *reqQue;               // The device request queue
    struct gendisk *gd_ptr;                     // Kernel’s representation of an individual disk device
    int totalSize;                              // Total size
} block_dev;

/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////
// PURPOSE - LKM init function
static int __init block_driver_init(void);
// PURPOSE - LKM exit function
static void __exit block_driver_exit(void);
// PURPOSE - Allocate a gendisk struct and add it to the system
static int create_block_device(block_dev *bDev);
// PURPOSE - Remove the gendisk from the system
static void delete_block_device(block_dev *bDev);
// PURPOSE - 
// NOTE - Could be called from user space or kernel space.  There's no way to know.
//  User Space Usage - partitioning a disk, building a filesystem, running a filesystem check
//  Kernel Space Usage - mount operation
static int open_block_device(struct block_device *bdev, fmode_t mode);
// PURPOSE - 
static int release_block_device(struct gendisk *gd, fmode_t mode);

/////////////
/* GLOBALS */
/////////////
int major_number = DEV_MAJOR_NUM;               // Will store our major number - returned by register_blkdev()
block_dev thisBDev;                             // Will store information about this block device
struct block_device_operations blockDevOps = {
    .owner = THIS_MODULE,
    .open = open_block_device,
    .release = release_block_device
};



//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////
static int __init block_driver_init(void)
{
    int retVal = 0;

    // 1. Register a new block device
    retVal = register_blkdev(major_number, DEVICE_NAME);

    if (retVal > 0)
    {
        printk(KERN_INFO "%s: Loaded module with major number %d\n", DEVICE_NAME, retVal);
        major_number = retVal;
        retVal = 0;
    }
    else
    {
        HARKLE_KERROR(DEVICE_NAME, register_blkdev, retVal);
    }

    // 2. Allocate and add disk to the system
    if (0 == retVal)
    {
        retVal = create_block_device(&thisBDev);
    }

    return retVal;
}


static void __exit block_driver_exit(void)
{
    // 1. Delete the disk from the system
    delete_block_device(&thisBDev);
    printk(KERN_INFO "%s: Deleted disk\n", DEVICE_NAME);

    // 2. Unregister it from the system
    unregister_blkdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "%s: Unregistered disk\n", DEVICE_NAME);
    printk(KERN_INFO "%s: Unloaded module\n", DEVICE_NAME);

    return;
}


static int create_block_device(block_dev *bDev)
{
    int retVal = 0;

    if (bDev)
    {
        // Zeroize block_dev struct
        memset(bDev, 0, sizeof(block_dev));
        
        // Initialize block_dev struct members
        // 0. Size
        bDev->totalSize = DEV_TOTAL_SIZE;
        // 1. Allocate a disk
        bDev->gd_ptr = alloc_disk(DEV_MAX_MINORS);

        if (bDev->gd_ptr)
        {
            printk(KERN_INFO "%s: Allocated gendisk struct\n", DEVICE_NAME);
            
            // 2. Populate the gendisk struct
            bDev->gd_ptr->major = major_number;
            bDev->gd_ptr->first_minor = which * DEV_MAX_MINORS;  // which?  Which which?!
            bDev->gd_ptr->fops = &blockDevOps;  // Block device file operations struct pointer
            bDev->gd_ptr->queue = bDev->reqQue;  // Block device request queue
            bDev->gd_ptr->private_data = bDev;
            snprintf(bDev->gd_ptr->disk_name, 32, "virtBlockDev0%c", which + 'a');
            set_capacity(bDev->gd_ptr, bDev->totalSize/KERNEL_SECTOR_SIZE);
            
            // 3. Add that disk to the system
            add_disk(bDev->gd_ptr);
            printk(KERN_INFO "%s: Added gendisk to system\n", DEVICE_NAME);
        }
        else
        {
            HARKLE_KERROR(DEVICE_NAME, alloc_disk, 0);
            retVal = -1;
        }
    }
    else
    {
        retVal = -1;
    }

    return retVal;
}


static void delete_block_device(block_dev *bDev)
{
    if (bDev && bDev->gd_ptr)
    {
        del_gendisk(bDev->gd_ptr);
    }
}


module_init(block_driver_init);
module_exit(block_driver_exit);

////////////////////////
/* DRIVER INFORMATION */
////////////////////////
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph 'Makleford' Harkleroad");
MODULE_DESCRIPTION("A block device driver for a custom virtual block device");
MODULE_VERSION("0.1");  // Not yet releasable

/*
    1. After a call to del_gendisk(), the struct gendisk structure may continue to exist 
    (and the device operations may still be called) if there are still users (an open 
    operation was called on the device but the associated release operation has not been 
    called). One solution is to keep the number of users of the device and call the 
    del_gendisk() function only when there are no users left of the device.
 */

/*
    TODO:
        [ ] Incorporate DEV_TOTAL_SIZE into the Makefile's lkm recipe instead of using hard-coded macro
 */
