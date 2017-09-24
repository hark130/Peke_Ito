#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/blink0"  // /dev/blink%d (%d == minor number)
#define BUFF_SIZE 8

void eat_a_newline(void);

int main(void)
{
    int i;          // Incrementer
    int fd;         // File descriptor

    char ch = 't';    // User choice
    char writeBuff[BUFF_SIZE + 1] = { 0 };
    char readBuff[BUFF_SIZE + 1] = { 0 };

    fd = open(DEVICE, O_RDWR);

    if (fd == -1)
    {
        printf("File %s either does not exist or has been locked by another process\n", DEVICE);
        exit(fd);
    }
    else
    {
        while(ch != 'q')
        {
            puts("No commands implemented");
            puts("s = send");
            puts("q = quit");
            puts("Enter command: ");
            ch = getchar();
            eat_a_newline();

            switch (ch)
            {
                // Implement commands here
                // case '?':
                //     break;
                case 's':
                    writeBuff[0] = 'n';     // command code for "set rgb now"
                    writeBuff[2] = 255;     // Green
                    write(fd, writeBuff, sizeof(writeBuff));
                    break;
                case 'q':
                    puts("Exiting");
                    break;
                default:
                    puts("Command not recognized");
                    puts("Try again");
                    break;
            }
        }
    }

    if (fd)
    {
        close(fd);
    }

    return 0;
}


void eat_a_newline(void)
{
    char letter = 't';

    while (letter != '\n' && letter != EOF)
    {
        letter = getchar();
    }

    return;
}
