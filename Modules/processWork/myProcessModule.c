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
#define MAX_PROC_NAME_LEN 15                   // There appears to be a max length for process names?
// Sample OS-agnostic(?) process names to attempt to resolve
#define process1 "bash"
#define process2 "sshd"
#define process3 "systemd"
#define PID1 1                                  // systemd?
#define PID2 2                                  // kthreadd?
#define PID3 4                                  // kworker?

/////////////
/* HEADERS */
/////////////
#include "HarkleKerror.h"                       // Kernel error macros
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED
#include <linux/sched/signal.h>                 // for_each_process
#include <linux/string.h>                       // memset(), strncmp()

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
// RETURN - Num of processes on success, -1 on error
int log_processes(void);
// PURPOSE - Resolve a process name into a PID
// RETURN - PID on success, -1 on error
int name_lookup(char *procName);
// PURPOSE - Resolve a PID to a process name
// RETURN - Name on success, NULL if not found, NULL on error
const char *PID_lookup(int PIDnum);

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
    int tempRet = 0;  // Return value from some process-related functions
    const char *tmp_ptr = NULL;  // Return value from PID_lookup
    
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
    // 1. List all processes
    tempRet = log_processes();
    if (-1 == tempRet)
    {
        HARKLE_KERROR(DEVICE_NAME, kernel_module_init, "log processes() failed");
    }
    else
    {
        printk(KERN_INFO "%s: There are currently %d running processes\n", DEVICE_NAME, tempRet);
    }

    // 2. Resolve some names
    tempRet = name_lookup(process1);
    printk(KERN_INFO "%s: Process '%s' has PID %d\n", DEVICE_NAME, process1, tempRet);
    tempRet = name_lookup(process2);
    printk(KERN_INFO "%s: Process '%s' has PID %d\n", DEVICE_NAME, process2, tempRet);
    tempRet = name_lookup(process3);
    printk(KERN_INFO "%s: Process '%s' has PID %d\n", DEVICE_NAME, process3, tempRet);

    // 3. Resolve PIDs
    tmp_ptr = PID_lookup(PID1);
    if (tmp_ptr) { printk(KERN_INFO "%s: PID %d is named '%s'\n", DEVICE_NAME, PID1, tmp_ptr); } else { HARKLE_KERROR(DEVICE_NAME, kernel_module_init, "log PID_lookup() failed"); }
    tmp_ptr = PID_lookup(PID2);
    if (tmp_ptr) { printk(KERN_INFO "%s: PID %d is named '%s'\n", DEVICE_NAME, PID2, tmp_ptr); } else { HARKLE_KERROR(DEVICE_NAME, kernel_module_init, "log PID_lookup() failed"); }
    tmp_ptr = PID_lookup(PID3);
    if (tmp_ptr) { printk(KERN_INFO "%s: PID %d is named '%s'\n", DEVICE_NAME, PID3, tmp_ptr); } else { HARKLE_KERROR(DEVICE_NAME, kernel_module_init, "log PID_lookup() failed"); }

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

int name_lookup(char *procName)
{
    // LOCAL VARIABLES
    int retVal = 0;
    struct task_struct *task;
    size_t procNameLen = 0;  // Length of procName

    // INPUT VALIDATION
    if (!procName)
    {
        retVal = -1;
        HARKLE_KERROR(DEVICE_NAME, name_lookup, "NULL pointer");
    }
    else if (!(*procName))
    {
        retVal = -1;
        HARKLE_KERROR(DEVICE_NAME, name_lookup, "Empty process name");
    }
    else
    {
        procNameLen = strlen(procName);

        for_each_process(task)
        {
            if (task->comm)
            {
                if (!strncmp(procName, task->comm, procNameLen))
                {
                    retVal = task->pid;  // Found it
                    break;  // Stop looking
                }
            }
        }
    }

    return retVal;
}

const char *PID_lookup(int PIDnum)
{
    // LOCAL VARIABLES
    char *retVal = NULL;
    struct task_struct *task;

    // INPUT VALIDATION
    if (PIDnum < 1)
    {
        HARKLE_KERROR(DEVICE_NAME, PID_lookup, "Invalid PID");
    }
    else
    {
        for_each_process(task)
        {
            if (task->comm)
            {
                if (task->pid == PIDnum)
                {
                    retVal = task->comm;  // Found it
                    break;  // Stop looking
                }
            }
        }

        if (!retVal)
        {
            HARKLE_KWARNG(DEVICE_NAME, PID_lookup, "Unable to find PID");
        }
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
