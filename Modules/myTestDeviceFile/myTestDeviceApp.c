#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define DEVICE "/dev/myTestDevice"
#define BUFF_SIZE 100

void eat_a_newline(void);

int main(void)
{
    int i;          // Incrementer
    int fd;         // File descriptor

    char ch = 't';    // User choice
    char writeBuff[BUFF_SIZE] = {0};
    char readBuff[BUFF_SIZE] = {0};

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
            puts("r = read from device");
            puts("w = write to device");
            puts("q = quit");
            puts("Enter command: ");
            ch = getchar();
            eat_a_newline();

            switch (ch)
            {
                case 'w':
                    printf("Enter data: ");
                    if (fgets(writeBuff, BUFF_SIZE, stdin) != writeBuff)
                    {
                        puts("Error reading from user input");
                        exit(-1);
                    }
                    else
                    {
                        eat_a_newline();
                        write(fd, writeBuff, sizeof(writeBuff));
                    }
                    break;
                case 'r':
                    read(fd, readBuff, sizeof(readBuff));
                    printf("DEVICE: %s\n", readBuff);
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
