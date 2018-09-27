/*
    REPO:        Peke_Ito (https://github.com/hark130/Peke_Ito/)
    FILE:        HarkleKerror.h
    PURPOSE:     Define conditionally-compiled kernel debugging MACROS
    DATE:        Updated 20180922
    VERSION:     0.1.0
 */

#include <linux/printk.h>                       // printk()

#ifndef __HARKLEKERROR__
#define __HARKLEKERROR__

#define HARKLE_KDEBUG  // Comment this out to turn off DEBUGGING MACROS

#ifdef HARKLE_KDEBUG
#define HARKLE_KERROR(module, funcName, errMsg) do { printk(KERN_ERR "%s: <<<ERROR>>> %s - %s\n", module, #funcName, errMsg); } while (0);
#define HARKLE_KERRNO(module, funcName, errNum) do { printk(KERN_ERR "%s: <<<ERRNO>>> %s() returned errno: %d!\n", module, #funcName, errNum); } while (0);
#define HARKLE_KWARNG(module, funcName, warnMsg) do { printk(KERN_WARNING "%s: ¿¿¿WARNING??? %s() - %s!\n", module, #funcName, warnMsg); } while (0);
#define HARKLE_KFINFO(module, funcName, msg) do { printk(KERN_INFO "%s: %s - %s\n", module, #funcName, msg); } while (0);
#define HARKLE_KINFO(module, msg) do { printk(KERN_INFO "%s: %s\n", module, msg); } while (0);
#else
#define HARKLE_KERROR(module, funcName, errMsg) ;;;
#define HARKLE_KERRNO(module, funcName, errNum) ;;;
#define HARKLE_KWARNG(module, funcName, warnMsg) ;;;
#define HARKLE_KFINFO(module, funcName, msg) ;;;
#define HARKLE_KINFO(module, msg) ;;;
#endif  // HARKLE_KDEBUG

#endif  // __HARKLEKERROR__

/*
    CHANGE LOG:
        v0.1.0
            - Initial commit of HARKLE_KERROR
            - Adapted from HARKLE_ERROR
 */

/*
	KERN_EMERG
	KERN_ALERT
	KERN_CRIT
	KERN_ERR
	KERN_WARNING
	KERN_NOTICE
	KERN_INFO
	KERN_CONT
	KERN_DEBUG
 */