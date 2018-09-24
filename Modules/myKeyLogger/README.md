## KEY LOGGING LINUX KERNEL MODULE (LKM)

### TO DO
[X] Basic Driver Framework

    [X] 1. Makefile    
    [X] 2. Basic compilable shell
    
[ ] Basic Functionality

    [X] 1. Implement code translation
    [X] 2. Implement File I/O
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
- [Phrack - Writing Linux Kernel Keylogger](http://phrack.org/issues/59/14.html)
- [An Online Approach for Kernel-level Keylogger Detection and Defense](https://pdfs.semanticscholar.org/37ea/54247bb3bedd356f5171ee5f8e1a83662dfc.pdf)
- [Safely copies data between kernel and user space via a character device](https://0x00sec.org/t/linux-keylogger-and-notification-chains/4566)
- [Replace a key stroke](https://stackoverflow.com/questions/33836541/linux-kernel-how-to-capture-a-key-press-and-replace-it-with-another-key)

#### Kernel File I/O

- [How to write to a file from a kernel module](http://krishnamohanlinux.blogspot.com/2013/12/how-to-write-to-file-from-kernel-module.html)
- [vfs_write behavior in O_APPEND mode](https://stackoverflow.com/questions/35451081/linux-kernel-is-vfs-write-thread-safe)
- [file_pos_write updates file->f_pos](https://elixir.bootlin.com/linux/latest/source/fs/read_write.c#L584)

#### Interrupts

- [man 1 inb+](https://www.systutorials.com/docs/linux/man/1-inb/)

### NOTES

- ```modinfo myKeyLogger.ko```
- ```more /proc/interrupts```
- "Most important, the process on behalf of which an interrupt handler is executed must always stay in the TASK_RUNNING state, or a system freeze can occur. Therefore, interrupt handlers cannot perform any blocking procedure such as an I/O disk operation." Understanding the Linux Kernel, 3rd Edition - 4. Interrupts and Exceptions - 4.6. Interrupt Handling - I/O Interrupt Handling
- "To log keystrokes, we will use our own keyboard interrupt handler.  Under Intel architectures, the IRQ of the keyboard controlled is IRQ 1.  When receives a keyboard interrupt, our own keyboard interrupt handler read the scancode and keyboard status.  Keyboard events can be read and written via port 0x60(Keyboard data register) and 0x64(Keyboard status register)." Writing [a] Linux Kernel Keylogger 
Phrack Magazine, Issue 59, Article 14
