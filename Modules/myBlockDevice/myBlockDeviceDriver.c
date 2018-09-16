/*
 *	PURPOSE - A block device driver for a virtual block device of my own creation
 */

/////////////
/* HEADERS */
/////////////
#include <linux/module.h>       				// ALWAYS NEED
#include <linux/kernel.h> 						// ALWAYS NEED

////////////
/* MACROS */
////////////
#define DEVICE_NAME "Virtual Block Device 0"	// virtBlockDev0

/////////////
/* GLOBALS */
/////////////
int retVal;                          	      	// Will be used to hold return values of functions

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
	retVal = 0;  // Placeholder for necessary function calls

    // Log it
    printk(KERN_INFO "%s: loaded module\n", DEVICE_NAME);

    return retVal;
}


static void __exit block_driver_exit(void)
{
    // Log it
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
