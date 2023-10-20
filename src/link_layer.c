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

    printf("try #%d , trying to send again...\n", alarmCount);
}

void logByte(const char* action, unsigned char byte) {
    printf("%s: 0x%02X (%c)\n", action, byte, byte);
}

unsigned char computeBCC2(const unsigned char *data, int size) {
    unsigned char bcc2 = data[0];
    for (int i = 1; i < size; i++) {
        bcc2 ^= data[i];
    }
    return bcc2;
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
            buf[0] = 0x7E;  // flag
            buf[1] = 0x03;  // addr
            buf[2] = 0x03;  // control
            buf[3] = buf[1] ^ buf[2];   // BCC
            buf[4] = 0x7E;

            int bytes = write(fd, buf, 5);
    
            alarm(3);   
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
    buf2[0]=0x7E;
    buf2[1]=0x01;
    buf2[2]=0x07;
    buf2[3]=buf2[1]^buf2[2];
    buf2[4]=0x7E;

       // Returns after 5 chars have been input
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

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings

    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize, int number)
{
    unsigned char stuffedBuffer[MAX_STUFFED_PAYLOAD_SIZE];
    int stuffedLength = stuffBytes(buf, bufSize, stuffedBuffer);
    
    int frameSize = stuffedLength + 6;
    unsigned char *frame = (unsigned char *)malloc(frameSize);
    if (!frame) {
        perror("Memory allocation failed");
        return -1;
    }
    printf("\n\n\n\n==============\n NEW INFO\n==============\n\n\n\n");
     for(int x=0; x<bufSize; x++){
        logByte("sent", buf[x]);
     }
     

    //logByte("Sent", buf[0]);
    //logByte("Sent", buf[bufSize-1]);

    unsigned char controlField = (number == 0) ? 0x00 : 0x40; // Using frame size to determine control field

    frame[0] = FRAME_FLAG;
    frame[1] = ADDR_TX;
    frame[2] = controlField;
    frame[3] = ADDR_TX ^ controlField;
    memcpy(frame + 4, stuffedBuffer, stuffedLength);
    frame[frameSize - 2] = computeBCC2(buf, bufSize);
     printf("bcc2 = 0x%02X\n", frame[frameSize-2]);
    frame[frameSize - 1] = FRAME_FLAG;

    printf("size of frame sent: %d\n", frameSize);


    int bytesWritten = write(fd, frame, frameSize);
    if (bytesWritten < 0) {
        perror("Failed to write to serial port");
        return -1;
    }
    //for (int i = 0; i < bytesWritten; i++) {
    //logByte("Sent", frame[i]);
    //}
    int bytess = 1;
    int j = 0;
    unsigned char buf3[1];
    unsigned char buf2[5];
    //memset 
    while(j<5){
        bytess = read(fd, buf3,1);
        if(bytess<=0){
            break;
        }
        //logByte("buffer", buf3[0]);
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
                    if((number==0 && buf2[i]==0x85) || (number==1 && buf2[i]==0x05)){
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
                    free(frame);
                    return bytesWritten;
                    }
                else{
                    break;
                }        
                }

    }
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char receivedBuffer[MAX_FRAME_SIZE];
    int receivedLength;
    unsigned char frame[MAX_FRAME_SIZE];
    int bytesRead = 0;
    int byte;
    unsigned char buf2[5];
    unsigned char buf[1];
    buf2[0]=0x7E;
    buf2[1]=0x01;
    buf2[2]=0x85;
    buf2[3]=buf2[1]^buf2[2];
    buf2[4]=0x7E;

    byte = read(fd, frame, 1);
    if (byte == -1 || frame[0] != 0x7E) {
        printf("Error receiving data packet \n");
        return -1; // error or no data read
    }
    //logByte("Received", frame[0]);
    bytesRead++;
    int j=1;
    while(1){
        byte = read(fd, buf, 1);
        if (byte == -1) {
            printf("Err0r receiving data packet \n");
            return -1; // error or no data read
        }
        //logByte("Received", frame[j]);
        
        frame[j]=buf[0];
        if(buf[0]==0x7E){
            printf("found it the flag: %d", j);
            logByte("Received", frame[j]);
            break;
        }
        bytesRead++;
        j++;
    }
    receivedLength = bytesRead;
    printf("Size of frame received: %d\n", bytesRead);
    if (ADDR_TX != frame[1]) {
        printf("Address check failed\n");
        return -1; // header BCC check failed
    }

    if (0x00 != frame[2]) {
        if(0x40 != frame[2]){
        printf("control check failed\n");
        return -1; // header BCC check failed
        }
    }

    if(frame[2]==0x40){
        buf2[2]=RR0;
    }

    unsigned char bcc1 = frame[1] ^ frame[2];
    if (bcc1 != frame[3]) {
        printf("BCC1 check failed\n");
        return -1; // header BCC check failed
    }

    int destuffedLength = destuffBytes(frame + 4, bytesRead - 5, packet); // skipping the flags, address, control and BCCs
    if(destuffedLength == -1) {
        printf("Error during byte destuffing.\n");
        return -1;
    }

    unsigned char receivedBcc2 = frame[bytesRead-1];
    unsigned char calculatedBcc2 = computeBCC2(packet, bytesRead - 5);
     printf("bcc2= 0x%02X\n", frame[bytesRead-1]);
    if (receivedBcc2 != calculatedBcc2) {
        printf("BCC2 check failed\n");
        for(int x=0; x<destuffedLength; x++){
        logByte("received", packet[x]);
        }
        logByte("received", packet[0]);
        logByte("received", packet[destuffedLength-1]);
        printf("Calculated BCC2: 0x%02X\n", calculatedBcc2);
        
        return -1;
    }
    
    byte = write(fd, buf2, 5);
    if(byte < 0) {
        perror("Failed to write to serial port");
        return -1;
    }

    // If everything is good, copy the data to the packet
    return bytesRead;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
