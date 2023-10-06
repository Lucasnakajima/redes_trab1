// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

volatile int STOP = FALSE;

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 30; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char buf2[5];

    //for (int i = 0; i < BUF_SIZE; i++)
    //{
        //buf[i] = 'a' + i % 26;
    //}

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    //buf[5] = '\n';
    
    (void)signal(SIGALRM, alarmHandler);
    while (alarmCount < 4 && STOP == FALSE) {
        if (alarmEnabled == FALSE) {
            buf[0] = 0x7E;  // flag
            buf[1] = 0x03;  // addr
            buf[2] = 0x03;  // control
            buf[3] = buf[1] ^ buf[2];   // BCC
            buf[4] = 0x7E;

            int bytes = write(fd, buf, BUF_SIZE);
            printf("%d bytes written\n", bytes);
    
            alarm(4);   
            alarmEnabled = TRUE;
        }
        int bytess = 1;
        int j = 0;
        unsigned char buf3[1];
        //memset 
        while(1){
            bytess = read(fd, buf3,1);
            if(bytess<=0){
                break;
            }
            buf2[j] = buf3[0];
            j++;
        }     
            for (int i = 0; i < sizeof(buf2); i++){
                if(i==0){
                    if(buf2[i]==0x7E){
                    }
                else{
                    break;
                }        
                }
                if(i==1){
                    if(buf2[i]==0x01){
                    }
                else if(buf2[i]==0x7E){
                i = -1;
                }
                else{
                    break;
                }        
                }
                if(i==2){
                    if(buf2[i]==0x07){
                    }
                else if(buf2[i]==0x7E){
                i = -1;
                }
                else{
                    break;
                }        
                }
                if(i==3){
                    if(buf2[i]==buf2[1]^buf2[2]){
                    }
                else{
                    break;
                }        
                }
                if(i==4){
                    if(buf2[i]==0x7E){
                    for (int j = 0; j < 5; j++){ 
                        printf("var %d = 0x%02X\n", j, buf2[j]);
                        }
                    alarm(0);
                    STOP = TRUE; 
                    }
                else{
                    break;
                }        
                }
            }
        
               
        }
    
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
