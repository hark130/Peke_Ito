/*
    REPO:        Peke_Ito (https://github.com/hark130/Peke_Ito/)
    FILE:        myKeyLoggingModule.c
    PURPOSE:     A block device driver for a virtual block device of my own creation
    DATE:        Updated 20180925
    VERSION:     1.0.0
 */

////////////
/* MACROS */
////////////

// GENERAL //
#define DEVICE_NAME "My Process Module"        // Use this macro for logging

/////////////
/* HEADERS */
/////////////
#include "HarkleKerror.h"                       // Kernel error macros
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED
#include <linux/string.h>                       // memset()

//////////////
/* TYPEDEFS */
//////////////
typedef struct _myProcDevice
{
    void *placeHolder;
} myProcDevice, *myProcDevice_ptr;

/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////
// PURPOSE - LKM init function
static int __init kernel_module_init(void);
// PURPOSE - LKM exit function
static void __exit kernel_module_exit(void);

/////////////
/* GLOBALS */
/////////////
myProcDevice myPD;

//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////
static int __init kernel_module_init(void)
{
    int retVal = 0;
    
    HARKLE_KINFO(DEVICE_NAME, "Process module loading");  // DEBUGGING

    ////////////////////////
    // Initialize Globals //
    ////////////////////////
    // 1. Initialize myProcDevice struct
    if (&myPD != memset(&myPD, 0x0, sizeof(myProcDevice)))
    {
        HARKLE_KERROR(DEVICE_NAME, kernel_module_init, "memset() failed");
    }

    //////////
    // DONE //
    //////////
    HARKLE_KINFO(DEVICE_NAME, "Process module loaded");  // DEBUGGING
    return retVal;
}

static void __exit kernel_module_exit(void)
{
    HARKLE_KINFO(DEVICE_NAME, "Process module unloading");  // DEBUGGING

    //////////
    // DONE //
    //////////
    HARKLE_KINFO(DEVICE_NAME, "Process module unloaded");  // DEBUGGING
    return;
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);

////////////////////////
/* DRIVER INFORMATION */
////////////////////////
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph 'Makleford' Harkleroad");
MODULE_DESCRIPTION("A basic process management Linux Kernel Module");
MODULE_VERSION("0.1"); // Not releasable
