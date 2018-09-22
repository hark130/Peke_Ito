/*
 *    PURPOSE - A block device driver for a virtual block device of my own creation
 */

/////////////
/* HEADERS */
/////////////
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED

////////////
/* MACROS */
////////////
#define DEVICE_NAME "My Key Logger"            // Use this macro for logging
#define HARKLE_KERROR(module, funcName, errNum) do { printk(KERN_ERR "%s: <<<ERROR>>> %s() returned %d!\n", module, #funcName, errNum); } while (0);
#define HARKLE_KINFO(module, msg) do { printk(KERN_INFO "%s: %s\n", module, msg); } while (0);

//////////////
/* TYPEDEFS */
//////////////
// typedef struct?

/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////
// PURPOSE - LKM init function
static int __init key_logger_init(void);
// PURPOSE - LKM exit function
static void __exit key_logger_exit(void);

/////////////
/* GLOBALS */
/////////////
// global struct?

//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////
static int __init key_logger_init(void)
{
    int retVal = 0;
    
    HARKLE_KINFO(DEVICE_NAME, "Key logger loading");
    // Code here
    
    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger loaded");
    return retVal;
}

static void __exit key_logger_exit(void)
{
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloading");
    // Code here
    
    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloaded");
}

module_init(key_logger_init);
module_exit(key_logger_exit);

////////////////////////
/* DRIVER INFORMATION */
////////////////////////
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph 'Makleford' Harkleroad");
MODULE_DESCRIPTION("A basic key logging Linux Kernel Module");
MODULE_VERSION("0.1"); // Not yet releasable
