## KEY LOGGING LINUX KERNEL MODULE (LKM)

### TO DO
[X] Basic Driver Framework

    [X] 1. Makefile    
    [X] 2. Basic compilable shell
    
[ ] Basic Functionality

    [X] 1. Implement code translation
    [ ] 2. Implement File I/O
    [ ] 3. Print keystroke
    [ ] 4. Save keystroke to file
    
[ ] Enhanced Functionality

    [ ] 1. Take an absolute filename as a parameter
    [ ] 2. [Create a /dev/ to provide output](http://cs.smith.edu/~nhowe/262/oldlabs/module.html)
    
[ ] Advanced Functionality

    [ ] * Support for multiple CPUs(?)
    [ ] * Allow file operations to append instead of overwriting

### SPECIFIC RESOURCES

#### Key Loggers

- [Find the section on key loggers](derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)
- [jarun/keysniffer saves to file](https://github.com/jarun/keysniffer/blob/master/keysniffer.c)
- [b1uewizard/linux-keylogger saves to file w/ good logging](https://github.com/b1uewizard/linux-keylogger/blob/master/kb.c)
- [arunpn123/keylogger appears to handle shift keys](https://github.com/arunpn123/keylogger/blob/master/keylogger.c)
- [CydGy/kernelmodule-keylogger uses header file for key maps and semaphores](https://github.com/CydGy/kernelmodule-keylogger/blob/master/keylogger.c)
- [kernc/logkeys deep solution](https://github.com/kernc/logkeys)
- [faineance/keyer hides itself?!](https://github.com/faineance/keyer)
- [jemtucker/qlog has last events?](https://github.com/jemtucker/qlog/blob/master/src/keylogger.c)

#### Kernel File I/O

- [How to write to a file from a kernel module](http://krishnamohanlinux.blogspot.com/2013/12/how-to-write-to-file-from-kernel-module.html)

### NOTES

- ```modinfo myKeyLogger.ko```
- ```more /proc/interrupts```
