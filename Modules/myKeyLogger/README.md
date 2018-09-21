## KEY LOGGING LINUX KERNEL MODULE (LKM)

### TO DO
[ ] Basic Driver Framework

    [ ] 1. Makefile    
    [ ] 2. Basic compilable shell
    
[ ] Basic Functionality

    [ ] 1. Print keystroke
    [ ] 2. Save keystroke to file
    
[ ] Enhanced Functionality

    [ ] 1. Take an absolute filename as a parameter
    [ ] 2. Create a /dev/ to provide output
    
[ ] Advanced Functionality

    [ ] 1. Support for multiple CPUs(?)

### SPECIFIC RESOURCES

- [Find the section on key loggers](derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)
- [jarun/keysniffer saves to file](https://github.com/jarun/keysniffer/blob/master/keysniffer.c)
- [b1uewizard/linux-keylogger saves to file w/ good logging](https://github.com/b1uewizard/linux-keylogger/blob/master/kb.c)
- [arunpn123/keylogger appears to handle shift keys](https://github.com/arunpn123/keylogger/blob/master/keylogger.c)
- [CydGy/kernelmodule-keylogger uses header file for key maps and semaphores](https://github.com/CydGy/kernelmodule-keylogger/blob/master/keylogger.c)
- [kernc/logkeys deep solution](https://github.com/kernc/logkeys)
- [faineance/keyer hides itself?!](https://github.com/faineance/keyer)
- [jemtucker/qlog has last events?](https://github.com/jemtucker/qlog/blob/master/src/keylogger.c)

### NOTES

- ```modinfo myKeyLogger.ko```
- ```more /proc/interrupts```
