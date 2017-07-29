#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>  // 1.

// 2.
int param_var = 0;
// Pass a value into this variable through:
//  insmod Hello.ko param_var = 1337
int param_arr[3] = {0, 0, 0};
// Pass a value into this array through:
//  insmod Hello.ko param_arr = 31,3,37

// 3. Register the variable
//module_param(name_var, type, permissions)
/*
    S_IRUSR     // Read User
    S_IWUSR     // Write User
    S_IXUSR     // Execute User
    S_IWGRP     // Write Group
    S_IRGRP     // Read Group

    S_IRUSR | S_IWUSR
 */
module_param(param_var, int, S_IRUSR | S_IWUSR);
module_param_array(param_arr, int, NULL, S_IRUSR | S_IWUSR);

void display(void)
{
    printk(KERN_ALERT "TEST: param = %d\n", param_var);
    printk(KERN_ALERT "TEST: arr[0] = %d\n", param_arr[0]);
    printk(KERN_ALERT "TEST: arr[1] = %d\n", param_arr[1]);
    printk(KERN_ALERT "TEST: arr[2] = %d\n", param_arr[2]);
}

// NOTES:
//  Static because there can be only one?
static int hello_init(void)
{
    printk(KERN_ALERT "TEST: Hello World!\n");
    display();
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

/* DESCRIPTIVE INFORMATION ABOUT THIS MODULE */
MODULE_AUTHOR("Edward Makleford");
MODULE_DESCRIPTION("My first LKM");
MODULE_LICENSE("GNU GPL v3.0");
