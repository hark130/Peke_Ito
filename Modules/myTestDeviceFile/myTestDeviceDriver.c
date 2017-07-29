#include <linux/module.h>       // ALWAYS NEED
#include <linux/kernel.h>       // ALWAYS NEED
#include <linux/fs.h>           // file_operations structure - allows use to open/close, read/write to device
#include <linux/cdev.h>         // This is a char driver, makes cdev available
#include <linux/semaphore.h>    // used to access semaphores, synchronization behaviors
#include <asm/uacess.h>         // copy_to_user, copy_from_user
