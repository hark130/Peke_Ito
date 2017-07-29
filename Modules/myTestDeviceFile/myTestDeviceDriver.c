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
