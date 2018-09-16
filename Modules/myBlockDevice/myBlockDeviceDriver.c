/*
 *    PURPOSE - A block device driver for a virtual block device of my own creation
 */

/////////////
/* HEADERS */
/////////////
#include <linux/fs.h>                           // Defines file table structures
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED

////////////
/* MACROS */
////////////
#define DEVICE_NAME "Virtual Block Device 0"    // virtBlockDev0
#define DEV_MAJOR_NUM 1337                      // Major number
#define DEV_MINOR_NUM 31337                     // Minor number
#define HARKLE_KERROR(module, funcName, errNum) do { printk(KERN_ERR "%s: <<<ERROR>>> %s() returned %d!\n", #module, #funcName, errNum); } while (0);

/////////////
/* GLOBALS */
/////////////
int retVal;                                     // Will be used to hold return values of functions
int major_number;                               // Will store our major number - returned by register_blkdev()

/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////
static int __init block_driver_entry(void);
static void __exit block_driver_exit(void);

//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////
static int __init block_driver_entry(void)
{
    // Register a new block device
    // retVal = register_blkdev(0, DEVICE_NAME);
    retVal = register_blkdev(DEV_MAJOR_NUM, DEVICE_NAME);

    if (retVal < 0)
    {
        HARKLE_KERROR(DEVICE_NAME, register_blkdev, retVal);
    }
    else
    {
        printk(KERN_INFO "%s: loaded module with major number %d\n", DEVICE_NAME, retVal);
    }    

    major_number = retVal;

    return retVal;
}


static void __exit block_driver_exit(void)
{
    unregister_blkdev(DEV_MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "%s: unloaded module\n", DEVICE_NAME);

    return;
}


module_init(block_driver_entry);
module_exit(block_driver_exit);

////////////////////////
/* DRIVER INFORMATION */
////////////////////////
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph 'Makleford' Harkleroad");
MODULE_DESCRIPTION("A block device driver for a custom virtual block device");
MODULE_VERSION("0.1");  // Not yet releasable
