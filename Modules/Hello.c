#include <linux/init.h>
#include <linux/module.h>

// NOTES:
//  Static because there can be only one?
static int hello_init(void)
{
    printk(KERN_ALERT "TEST: Hello World!\n");

    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "TEST: Good night.\n");

    return;
}

// Tell the Linux kernel where to start executing your Linux module
// sudo insmod Hello.ko
module_init(hello_init);
// Verify the module is loaded with...
// cat /proc/modules | grep Hello
// Modules typically live in /lib/modules
// sudo rmmod Hello.ko
module_exit(hello_exit);
