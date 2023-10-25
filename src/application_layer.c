#include "application_layer.h"
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

speed_t getBaudRateConstant(int baudRate) {
    switch (baudRate) {
        case 9600:   return B9600;
        case 115200: return B115200;

        default: return -1;  
    }
}

ssize_t readDataFromFile(const char *filename, unsigned char *buffer, size_t limit) {
    static int fd = -1;  
    if(fd == -1) {
        fd = open(filename, O_RDONLY);
        if(fd < 0) {
            perror("Failed to open file");
            return -1;
        }
    }

    ssize_t bytesRead = read(fd, buffer, limit);
    if(bytesRead < 0) {
        perror("Failed to read from file");
        close(fd);
        fd = -1;
        return -1;
    }

    if(bytesRead == 0) {  
        close(fd);
        fd = -1;  
    }
    return bytesRead;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    if (!serialPort || !role || !filename)
    {
        printf("Invalid arguments provided.\n");
        return;
    }

    // Setup connection parameters for the link layer
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort,serialPort);
    connectionParameters.baudRate = getBaudRateConstant(baudRate);
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    // Set the role for the link layer
    if (strcmp(role, "tx") == 0)
    {
        connectionParameters.role = LlTx;
    }
    else if (strcmp(role, "rx") == 0)
    {
        connectionParameters.role = LlRx;
    }
    else
    {
        printf("Unknown role: %s. Please specify either 'tx' or 'rx'.\n", role);
        return;
    }

    // Open the connection
    if (llopen(connectionParameters) == -1)
    {
        printf("Failed to open the connection.\n");
        return;
    }

    // Transmitter logic
    if (connectionParameters.role == LlTx)
    {
        unsigned char buffer[MAX_PAYLOAD_SIZE];
        int bytesRead;
        int i=0;
        int j=0;

        while((bytesRead = readDataFromFile(filename, buffer, MAX_PAYLOAD_SIZE)) > 0) {
            if(llwrite(buffer, bytesRead, i) == -1) {
                printf("Failed to transmit data.\n");
                return;
            }
            if(i==0){
                i=1;
            }
            else{
                i=0;
            }
            j++;
        }
        printf("sent %d times \n", j);
        if(llclose(0) != 1){
          perror("Failed to close connection");
          return;  
        }

    }
    // Receiver logic
    else if (connectionParameters.role == LlRx)
    {
       unsigned char packet[MAX_PAYLOAD_SIZE];

    int fd_output = open(filename, O_WRONLY | O_CREAT, 0666);
    if (fd_output < 0) {
        perror("Failed to open output file");
        return;
    }

    while (1) {
        int bytesRead = llread(packet);
        if (bytesRead == -1) {
            printf("Err0r receiving data packet \n");
            break;
        }
        else if(bytesRead == 5){
            break;
        }
        bytesRead = bytesRead-6;
        if (write(fd_output, packet, bytesRead) < 0) {
            perror("Failed to write to file");
            close(fd_output);
            return;
        }
    }

    close(fd_output);
    printf("File received and saved as %s.\n", filename);
    }

    // Close the connection and show statistics
    if (llclose(TRUE) == -1)
    {
        printf("Failed to close the connection.\n");
    }
}
