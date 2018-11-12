#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "headers.h"

#define MAXWAIT 5    /*Maximum number of connection requests*/
char msgBuffer[256];	//Character buffer for reading from socket

void reportError(char *err);


int main(int argc, char *argv[])
{
    int serverFDT,clientFDT;    //Socket File Descriptor Tables
    int portNum;		//Server Port #
    socklen_t clientAddrLen; //Size of Client address (used in system call)
    int messageLen;		//Size of message sent between client/server
    struct sockaddr_in serverAddr; //Server Address
    struct sockaddr_in clientAddr; //Client Address    

    /*
     * Prompt user for port number to listen on and setup socket.
     */
    printf("Enter Listening Port Number: \n");
    scanf("%d", &portNum);


    /* 
     * Using AF_INET, Stream Reading, and let system decide on TCP/UDP by passing 0 
     */
    serverFDT = socket(AF_INET, SOCK_STREAM, 0);

    if(serverFDT < 0) //If value is negative, socket creation failed, exit and print error
    {
        reportError("Socket Initialization Failed!!"); 
    }

    /*Assign the given port number to the socket*/
    memset(&serverAddr, 0 , sizeof(serverAddr));   //Fill address with zeros
    serverAddr.sin_family = AF_INET;		   //Address family
    serverAddr.sin_port = htons(portNum);       // Set local port using Network Byte Order
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //Local IP address (Host IP)

    /*Bind Local Socket and exit with error if failure occurs.*/
    if(bind(serverFDT, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0)
    {
        reportError("Server Socket Bind Failed!!"); 
    }

    /*Setup server socket to listen for incoming connections.*/
    if(listen(serverFDT, MAXWAIT) < 0)
    {
        reportError("Error Listening!!");
    }

    /*Client Handling*/
    clientAddrLen =sizeof(clientAddr);

    /*Block program until a client connects*/
    clientFDT = accept(serverFDT, (struct sockaddr * ) &clientAddr, &clientAddrLen);
    if(clientFDT < 0)
    {
        reportError("Error Accepting Client Connection!");
    }

    printf("Client Connected!\n");
    memset(msgBuffer, 0 , 256);   //Fill buffer with zeros

    /*Get data and process*/
    messageLen = read(clientFDT,msgBuffer,255);
    
    if(messageLen < 0)
    {
     reportError("Error Receiving Data!");
    }

    /*Print Received Message*/
    printf("Message From Client: %s", msgBuffer);
    
    /*Send Message Back To Client*/
    messageLen = write(clientFDT,"Message Received Successfully!\n",31);

    if(messageLen < 0)
    {
     reportError("Error Sending Data!");
    }

    return 0;
}

/*Reports and error and exits the program*/
void reportError(char *err)
{
    perror(err);
    exit(1);
}
#include "headers.h"

void *producer(void *args) {
    shared_struct *ptr = ((pthread_args *) args)->ptr;
    FILE *fd = ((pthread_args *) args)->fd;
    item elem;
    
    int id  = 0;
    int end = 0;

    while (1) {
        elem.id = id++;
        // Read each line from the input file
	sem_wait(&ptr->empty);
	sem_wait(&ptr->mutex);

        if (strlen(msgBuffer)>0) {
	// put an item in buffer
        ptr->buffer[ptr->in] = elem;
        ptr->in = (ptr->in + 1) % BUFFER_SIZE;
        }
	sem_post(&ptr->full);
	sem_post(&ptr->mutex);
    }
	sem_post(&ptr->full);
	sem_post(&ptr->mutex);

}


