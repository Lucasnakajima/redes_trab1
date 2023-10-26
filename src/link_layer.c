// Link layer protocol implementation

#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = FALSE;
int alarmCount = 0;

int fd;

LinkLayer oficial;

#include <stdio.h>
#include <time.h>

static int correctlyReceivedFrameCount = 0; 
const double headerErrorProbability = 0.7; 
const double dataErrorProbability = 0.7;

volatile sig_atomic_t alarmTriggered = 0;

void propagationDelayHandler(int signo) {
    (void) signo; 
    alarmTriggered = 1;
}



int stuffBytes(const unsigned char* input, int inputSize, unsigned char* output) {
    int outputSize = 0;

    for(int i = 0; i < inputSize; i++) {

        if(input[i] == 0x7E) { // If the byte matches the flag
            output[outputSize++] = 0x7D; // Escape byte
            output[outputSize++] = 0x5E; // Result of XOR with 0x20
        } else if(input[i] == 0x7D) { // If the byte matches the escape byte
            output[outputSize++] = 0x7D; // Escape byte
            output[outputSize++] = 0x5D; // Result of XOR with 0x20
        } else {
            output[outputSize++] = input[i]; // Regular byte
        }
    }

    return outputSize;
}

int destuffBytes(const unsigned char* input, int inputSize, unsigned char* output) {
    int outputSize = 0;
    int i = 0;

    while(i < inputSize) {
        if(input[i] == 0x7D) { // Escape byte detected
            i++; // Move to next byte

            if(i >= inputSize) {
                printf("Error: Truncated escape sequence at end of input.\n");
                return -1; // Indicate error
            }

            if(input[i] == 0x5E) {
                output[outputSize++] = 0x7E; // Replace with 0x7e
            } else if(input[i] == 0x5D) {
                output[outputSize++] = 0x7D; // Replace with 0x7d
            } else {
                printf("Error: Invalid escape sequence 0x7d 0x%02x.\n", input[i]);
                return -1; // Indicate error
            }
        } else {
            output[outputSize++] = input[i];
        }

        i++;
    }

    return outputSize;
}

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    if(alarmCount == oficial.nRetransmissions){
        printf("try #%d failed...\n", alarmCount);
    }
    else{
    printf("try #%d failed , trying to send again...\n", alarmCount);
    }
}

unsigned char computeBCC2(const unsigned char *data, int size) {
    unsigned char bcc2 = data[0];
    for (int i = 1; i < size; i++) {
        bcc2 ^= data[i];
    }
    return bcc2;
}

void sendRejectionFrame(unsigned char *frame) {
    unsigned char buf2[5];
    buf2[0] = 0x7E;
    buf2[1] = 0x01;

    // Update control byte based on received frame's control byte
    if (frame[2] == 0x40) {
        buf2[2] = 0x81;
    } else {
        buf2[2] = 0x01;
    }

    buf2[3] = buf2[1] ^ buf2[2];
    buf2[4] = 0x7E;
    
    int byte = write(fd, buf2, 5);
    if (byte < 0) {
        perror("Failed to write to serial port");
        // Handle the error (either by returning or continuing as per your logic)
    }
}



////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        return -1;
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return -1;
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = connectionParameters.timeout*10; 
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);  

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    oficial = connectionParameters;

    printf("New Connection set\n");

    if(connectionParameters.role == LlTx){
    unsigned char buf[5] = {0};
    unsigned char buf2[5];
    volatile int STOP = FALSE;
         (void)signal(SIGALRM, alarmHandler);
        while (alarmCount < connectionParameters.nRetransmissions && STOP == FALSE) {
        if (alarmEnabled == FALSE) {
            buf[0] = FRAME_FLAG;  // flag
            buf[1] = ADDR_TX;  // addr
            buf[2] = 0x03;  // control
            buf[3] = buf[1] ^ buf[2];   // BCC
            buf[4] = FRAME_FLAG;

            int bytes = write(fd, buf, 5);
    
            alarm(connectionParameters.timeout);   
            alarmEnabled = TRUE;
        }
        int bytess = 1;
        int j = 0;
        unsigned char buf3[1];
        //memset 
        while(j<5){
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
        if(alarmCount == connectionParameters.nRetransmissions){
            return -1;
        }
    
    // Restore the old port settings
    }
    else{
    unsigned char buf[5];
    unsigned char buf2[5];
    buf2[0]=FRAME_FLAG;
    buf2[1]=ADDR_RX;
    buf2[2]=0x07;
    buf2[3]=buf2[1]^buf2[2];
    buf2[4]=FRAME_FLAG;

       
           int STOP=0;
        while(STOP==0){
        int bytess = 1;
        int j = 0;
        unsigned char buf3[1];
        //memset 
        while(j<5){
            bytess = read(fd, buf3,1);
            if(bytess<=0){
                break;
            }
            buf[j] = buf3[0];
            j++;
        }
        for(int i =0;i<5;i++){
            if(i==0){
              if(buf[i]==0x7E){}
            else break;            
            }
            if(i==1){
              if(buf[i]==0x03){}
            else if(buf[i]==0x7E){i= -1;}
            else break;            
            }
            if(i==2){
              if(buf[i]==0x03){}
            else if(buf[i]==0x7E){i= -1;}
            else break;            
            }
            if(i==3){
              if(buf[i]==buf[1]^buf[2]){}
            else break;            
            }
            if(i==0){
              if(buf[i]==0x7E){
              alarm(0);
              STOP=1;
              }
            else break;            
            }
            
        }
        
        }
    
    int bytes = write(fd, buf2, 5);
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize, int number)
{

    alarmCount = 0;
    alarmEnabled = FALSE;
    unsigned char stuffedBuffer[MAX_STUFFED_PAYLOAD_SIZE];
    int stuffedLength = stuffBytes(buf, bufSize, stuffedBuffer);

    int frameSize = stuffedLength + 6;
    unsigned char *frame = (unsigned char *)malloc(frameSize);
    if (!frame) {
        perror("Memory allocation failed");
        return -1;
    }

    unsigned char controlField = (number == 0) ? 0x00 : 0x40;

    frame[0] = FRAME_FLAG;
    frame[1] = ADDR_TX;
    frame[2] = controlField;
    frame[3] = ADDR_TX ^ controlField;
    memcpy(frame + 4, stuffedBuffer, stuffedLength);
    frame[frameSize - 2] = computeBCC2(buf, bufSize);
    frame[frameSize - 1] = FRAME_FLAG;

    (void)signal(SIGALRM, alarmHandler);

    int bytesWritten;
    unsigned char buf2[5];
    volatile int STOP = FALSE;

    while (alarmCount < oficial.nRetransmissions && STOP == FALSE) {
        if (alarmEnabled == FALSE) {
            bytesWritten = write(fd, frame, frameSize);
            if (bytesWritten < 0) {
                perror("Failed to write to serial port");
                free(frame);
                return -1;
            }
            alarm(oficial.timeout);
            alarmEnabled = TRUE;
        }

        int bytess = 1;
        int j = 0;
        unsigned char buf3[1];

        while(j < 5) {
            bytess = read(fd, buf3, 1);
            if (bytess <= 0) {
                break;
            }
            buf2[j] = buf3[0];
            j++;
        }
        for (int i = 0; i < 5; i++){
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
                    if((number==0 && buf2[i]==0x85) || (number==1 && buf2[i]==0x05)){
                    }
                else if(buf2[i]==0x7E){
                i = -1;
                }
                else if((number==0 && buf2[i]==0x01) || (number==1 && buf2[i]==0x81)){
                    break;
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
                    alarm(0);
                    STOP = TRUE;
                    }
                else{
                    break;
                }        
                }

        }
    }
    free(frame);
    if (alarmCount == oficial.nRetransmissions) {
        return -1;
    } else {
        return bytesWritten;
    }
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{

    /*
    alarmTriggered = 0;  
    
    
    signal(SIGALRM, propagationDelayHandler);
    alarm(1);  

    
    while (!alarmTriggered) {
        pause();  
    }

    */
    int receivedLength;
    
    int byte;
    unsigned char buf2[5];
    unsigned char buf[1];
    int destuffedLength;
    buf2[0]=0x7E;
    buf2[1]=0x01;
    buf2[2]=0x85;
    buf2[3]=buf2[1]^buf2[2];
    buf2[4]=0x7E;
    volatile int rejected = TRUE;

    while(rejected){
    unsigned char buffer[MAX_FRAME_SIZE];
    rejected = FALSE;
    unsigned char frame[MAX_FRAME_SIZE];
    int bytesRead = 1;
    byte = read(fd, frame, 1);
    if (byte == -1) {
        printf("Error reading data packet \n");
        return -1; // error or no data read
    }
    if(frame[0] != 0x7E){
        sendRejectionFrame(frame);
        rejected = TRUE;
        continue;
    }
    int j=1;
    volatile int close = FALSE;
    
    while(1){
        byte = read(fd, buf, 1);
        if (byte == -1) {
            printf("Error reading data\n");
            return -1; // error or no data read
        }
        if(j==2 && buf[0]==DISC){
            close=TRUE;
        }
        
        frame[j]=buf[0];
        bytesRead++;
        j++;
        if(buf[0]==0x7E){            
            break;
        }
        
    }
    receivedLength = bytesRead;
    if (ADDR_TX != frame[1]) {
        printf("Address check failed\n");
        sendRejectionFrame(frame);
        rejected = TRUE;
        continue;
    }
    if (0x00 != frame[2] && !close) {
        if(0x40 != frame[2]){
        printf("control check failed\n");
        sendRejectionFrame(frame);
        rejected = TRUE;
        continue;
        }
    }

    if(frame[2]==0x40 && !close){
        buf2[2]=RR0;
    }

    unsigned char bcc1 = frame[1] ^ frame[2];

    destuffedLength = destuffBytes(frame + 4, bytesRead - 6, packet);
    if(destuffedLength == -1) {
        printf("Error during byte destuffing.\n");
        return -1;
    }

    if(correctlyReceivedFrameCount >= 5) {
        if((rand() / (double) RAND_MAX) < headerErrorProbability) {
            bcc1 = bcc1 ^ 0x01; 
        }

        if((rand() / (double) RAND_MAX) < dataErrorProbability) {
            packet[rand() % destuffedLength] ^= 0x01;  
        }

        correctlyReceivedFrameCount = 0; 
    }

    if (bcc1 != frame[3]) {
        sendRejectionFrame(frame);
        rejected = TRUE;
        continue;
    }
    if(close){
        return 5;
    }

    
    

    unsigned char receivedBcc2 = frame[bytesRead-2];
    unsigned char calculatedBcc2 = computeBCC2(packet, destuffedLength);
    if (receivedBcc2 != calculatedBcc2) {
        sendRejectionFrame(frame);
        rejected = TRUE;
        continue;
    }
    }
    byte = write(fd, buf2, 5);
    if(byte < 0) {
        perror("Failed to write to serial port");
        return -1;
        }
    correctlyReceivedFrameCount++;
    return destuffedLength;
    
    
}


////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    unsigned char disc_tx[5];
    disc_tx[0]=FRAME_FLAG;
    disc_tx[1]=ADDR_TX;
    disc_tx[2]=DISC;
    disc_tx[3]=disc_tx[1]^disc_tx[2];
    disc_tx[4]=FRAME_FLAG;

    unsigned char ua_tx[5];
    ua_tx[0]=FRAME_FLAG;
    ua_tx[1]=ADDR_TX;
    ua_tx[2]=UA;
    ua_tx[3]=ua_tx[1]^ua_tx[2];
    ua_tx[4]=FRAME_FLAG;

    unsigned char disc_rx[5];
    unsigned char buff[1];
    unsigned char buf[5];
    disc_rx[0]=FRAME_FLAG;
    disc_rx[1]=ADDR_RX;
    disc_rx[2]=DISC;
    disc_rx[3]=disc_rx[1]^disc_rx[2];
    disc_rx[4]=FRAME_FLAG;
    alarmEnabled = FALSE;
    alarmCount = 0;
   
    if(oficial.role == LlTx){
        fd = open(oficial.serialPort, O_RDWR | O_NOCTTY);
        volatile int STOP = FALSE;
        (void) signal(SIGALRM, alarmHandler);
         while (alarmCount < oficial.nRetransmissions && STOP == FALSE) {
            if (alarmEnabled == FALSE) {
                int byte = write(fd, disc_tx, 5);
                if (byte < 0) {
                    perror("Failed to write to serial port");
                    return -1;
                }
                alarm(oficial.timeout);
                alarmEnabled = TRUE;
            }
        int j=0;

         while(j<5){
            int bytes = read(fd, buff,1);
            if(bytes<=0){
                break;
            }
            switch (j)
            {
            case 0:
                if(buff[0]==FRAME_FLAG){
                break;
                }
                printf("Error in flag check.\n");
                return -1;
            case 1:
                if(buff[0]==ADDR_RX){
                break;
                }
                printf("Error in address check.\n");
                return -1;
            case 2:
                if(buff[0]==DISC){
                break;
                }
                printf("Error in control check.\n");
                return -1;
            case 3:
                if(buff[0]== buf[1]^buf[2]){
                break;
                }
                printf("Error in bcc check.\n");
                return -1;
            case 4:
                if(buff[0]==FRAME_FLAG){
                break;
                }
                printf("Error in flag check.\n");
                return -1;
            
            default:
                return -1;
                break;
            }
            buf[j] = buff[0];
            j++;
            }
            alarm(0); 
            STOP = TRUE;
        }

        int byte = write(fd, ua_tx, 5);
        if(byte < 0) {
        perror("Failed to write to serial port");
        return -1;
        }
        close(fd);
        return 1;

    }

    else{
        int byte = write(fd, disc_rx, 5);
        int j=0;

         while(j<5){
            int bytes = read(fd, buff,1);
            if(bytes<=0){
                perror("Failed to read from serial port");
                return -1;
            }
            switch (j)
            {
            case 0:
                if(buff[0]==FRAME_FLAG){
                break;
                }
                printf("Error in flag check.\n");
                return -1;
            case 1:
                if(buff[0]==ADDR_TX){
                break;
                }
                printf("Error in address check.\n");
                return -1;
            case 2:
                if(buff[0]==UA){
                break;
                }
                printf("Error in control check.\n");
                return -1;
            case 3:
                if(buff[0]== buf[1]^buf[2]){
                break;
                }
                printf("Error in bcc check.\n");
                return -1;
            case 4:
                if(buff[0]==FRAME_FLAG){
                break;
                }
                printf("Error in flag check.\n");
                return -1;
            
            default:
                return -1;
                break;
            }
            buf[j] = buff[0];
            j++;
        }
        close(fd);
        return 1;
    }

    return -1;
}
