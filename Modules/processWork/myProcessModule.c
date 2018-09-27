/*
    REPO:        Peke_Ito (https://github.com/hark130/Peke_Ito/)
    FILE:        myProcessModule.c
    PURPOSE:     A basic process management Linux Kernel Module
    DATE:        Updated 20180926
    VERSION:     0.0.1
 */

////////////
/* MACROS */
////////////

// GENERAL //
#define DEVICE_NAME "My Process Module"        // Use this macro for logging
// Sample OS-agnostic(?) process names to attempt to resolve
#define process1 "bash"
#define process2 "sshd"
#define process3 "systemd"

/////////////
/* HEADERS */
/////////////
#include "HarkleKerror.h"                       // Kernel error macros
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED
#include <linux/sched/signal.h>                 // for_each_process
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
// PURPOSE - Log information about all running processes
// RETURN - Num of processes on success, -1 for error
int log_processes(void);

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
    int numProc = 0;  // Return value from log_processes
    
    HARKLE_KINFO(DEVICE_NAME, "Process module loading");  // DEBUGGING

    ////////////////////////
    // Initialize Globals //
    ////////////////////////
    // 1. Initialize myProcDevice struct
    if (&myPD != memset(&myPD, 0x0, sizeof(myProcDevice)))
    {
        HARKLE_KERROR(DEVICE_NAME, kernel_module_init, "memset() failed");
    }

    ////////////////////////
    // DO SOMETHING BASIC //
    ////////////////////////
    numProc = log_processes();

    if (-1 == numProc)
    {
        HARKLE_KERROR(DEVICE_NAME, kernel_module_init, "log processes() failed");
    }
    else
    {
        printk(KERN_INFO "%s: There are currently %d running processes\n", DEVICE_NAME, numProc);
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

int log_processes(void)
{
    int retVal = 0;
    struct task_struct *task;

    for_each_process(task)
    {
        printk(KERN_INFO "%s: %s [%d]\n", DEVICE_NAME, task->comm, task->pid);
        retVal++;
    }

    return retVal;
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
